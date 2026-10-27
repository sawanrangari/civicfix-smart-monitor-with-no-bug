"""
Citizen Complaint System - Street Light & Civic Issues
Roles: Citizen + Engineer
Stack: Flask + SQLite
"""
import os
import secrets
import sqlite3
import time
from datetime import datetime
from functools import wraps
from flask import Flask, render_template, request, redirect, url_for, session, flash, g, jsonify
from werkzeug.security import generate_password_hash, check_password_hash

app = Flask(__name__)

# Never use a known/default secret key in production. Render should provide SECRET_KEY
# through its Environment settings (the included render.yaml generates one).
_secret_key = os.environ.get("SECRET_KEY", "").strip()
_is_render = bool(os.environ.get("RENDER_SERVICE_ID")) or os.environ.get("RENDER", "").strip().lower() in {"1", "true", "yes"}
if _is_render and not _secret_key:
    raise RuntimeError("SECRET_KEY must be configured in Render Environment before starting CivicFix.")
app.secret_key = _secret_key or secrets.token_hex(32)  # local development only
app.config.update(
    SESSION_COOKIE_HTTPONLY=True,
    SESSION_COOKIE_SAMESITE="Lax",
    SESSION_COOKIE_SECURE=_is_render,
    MAX_CONTENT_LENGTH=5 * 1024 * 1024,
)

# Public sign-up is citizen-only. Existing engineer accounts continue to work.
ALLOW_ENGINEER_SIGNUP = os.environ.get("ALLOW_ENGINEER_SIGNUP", "0").strip().lower() in {"1", "true", "yes"}

@app.after_request
def add_security_headers(response):
    response.headers.setdefault("X-Content-Type-Options", "nosniff")
    response.headers.setdefault("X-Frame-Options", "SAMEORIGIN")
    response.headers.setdefault("Referrer-Policy", "strict-origin-when-cross-origin")
    response.headers.setdefault("Permissions-Policy", "geolocation=(self), camera=(self), microphone=()")
    # Render terminates HTTPS at its proxy; trust the forwarded scheme only for HSTS.
    forwarded_proto = request.headers.get("X-Forwarded-Proto", "").split(",", 1)[0].strip().lower()
    if request.is_secure or (_is_render and forwarded_proto == "https"):
        response.headers.setdefault("Strict-Transport-Security", "max-age=31536000")
    if request.path in {"/login", "/register"} or request.path.startswith(("/citizen", "/engineer")):
        response.headers["Cache-Control"] = "no-store"
    return response

START_TIME = time.time()

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATABASE = os.path.join(BASE_DIR, "complaints.db")
UPLOAD_FOLDER = os.path.join(BASE_DIR, "static", "uploads")
ALLOWED_EXTENSIONS = {"png", "jpg", "jpeg", "gif", "webp"}
app.config["UPLOAD_FOLDER"] = UPLOAD_FOLDER
app.config["MAX_CONTENT_LENGTH"] = 5 * 1024 * 1024  # 5 MB
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

CATEGORIES = ["Street Light", "Water Supply", "Road Damage", "Garbage", "Drainage", "Manhole", "Other"]
STATUSES = ["Pending", "In Progress", "Completed"]


def get_db():
    if "db" not in g:
        g.db = sqlite3.connect(DATABASE)
        g.db.row_factory = sqlite3.Row
    return g.db


@app.teardown_appcontext
def close_db(e=None):
    db = g.pop("db", None)
    if db is not None:
        db.close()


def init_db():
    db = sqlite3.connect(DATABASE)
    db.execute("""
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            email TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL,
            role TEXT NOT NULL CHECK(role IN ('citizen','engineer')),
            created_at TEXT NOT NULL
        )
    """)
    db.execute("""
        CREATE TABLE IF NOT EXISTS complaints (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            citizen_id INTEGER NOT NULL,
            category TEXT NOT NULL,
            title TEXT NOT NULL,
            description TEXT NOT NULL,
            area TEXT NOT NULL,
            status TEXT NOT NULL DEFAULT 'Pending',
            remark TEXT DEFAULT '',
            image_path TEXT DEFAULT '',
            latitude REAL DEFAULT NULL,
            longitude REAL DEFAULT NULL,
            other_detail TEXT DEFAULT '',
            source TEXT NOT NULL DEFAULT 'citizen',
            device_name TEXT DEFAULT '',
            trigger_key TEXT DEFAULT '',
            sensor_data TEXT DEFAULT '{}',
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL,
            FOREIGN KEY (citizen_id) REFERENCES users(id)
        )
    """)
    # Migrate old DBs missing new columns
    cols = [r[1] for r in db.execute("PRAGMA table_info(complaints)").fetchall()]
    if "image_path" not in cols:
        db.execute("ALTER TABLE complaints ADD COLUMN image_path TEXT DEFAULT ''")
    if "latitude" not in cols:
        db.execute("ALTER TABLE complaints ADD COLUMN latitude REAL DEFAULT NULL")
    if "longitude" not in cols:
        db.execute("ALTER TABLE complaints ADD COLUMN longitude REAL DEFAULT NULL")
    if "other_detail" not in cols:
        db.execute("ALTER TABLE complaints ADD COLUMN other_detail TEXT DEFAULT ''")
    if "source" not in cols:
        db.execute("ALTER TABLE complaints ADD COLUMN source TEXT NOT NULL DEFAULT 'citizen'")
    if "device_name" not in cols:
        db.execute("ALTER TABLE complaints ADD COLUMN device_name TEXT DEFAULT ''")
    if "trigger_key" not in cols:
        db.execute("ALTER TABLE complaints ADD COLUMN trigger_key TEXT DEFAULT ''")
    if "sensor_data" not in cols:
        db.execute("ALTER TABLE complaints ADD COLUMN sensor_data TEXT DEFAULT '{}'")
    db.execute("""
        CREATE TABLE IF NOT EXISTS esp_alert_state (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_name TEXT NOT NULL,
            trigger_key TEXT NOT NULL,
            active INTEGER NOT NULL DEFAULT 0,
            last_seen TEXT NOT NULL,
            last_complaint_id INTEGER DEFAULT NULL,
            UNIQUE(device_name, trigger_key)
        )
    """)
    db.execute("""
        CREATE TABLE IF NOT EXISTS esp_devices (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            ip TEXT NOT NULL,
            area TEXT DEFAULT '',
            added_by INTEGER,
            created_at TEXT NOT NULL
        )
    """)
    db.commit()
    db.close()


def allowed_file(filename):
    return "." in filename and filename.rsplit(".", 1)[1].lower() in ALLOWED_EXTENSIONS


# ---------- AI: repeat-location insight (rule-based, no API key needed) ----------
import math
import re as _re

def normalize_area(s):
    s = (s or "").lower().strip()
    s = _re.sub(r"\s+", " ", s)
    return s

def haversine_km(lat1, lon1, lat2, lon2):
    try:
        R = 6371.0
        p1, p2 = math.radians(lat1), math.radians(lat2)
        dphi = math.radians(lat2 - lat1)
        dl = math.radians(lon2 - lon1)
        a = math.sin(dphi/2)**2 + math.cos(p1)*math.cos(p2)*math.sin(dl/2)**2
        return 2 * R * math.asin(math.sqrt(a))
    except Exception:
        return None

AI_CAUSES = {
    "Street Light": [
        "Voltage fluctuation / loose neutral in this lane fuses bulbs repeatedly.",
        "Aged fitting or water ingress in the pole cap causes frequent failure.",
        "Same faulty batch of bulbs or timer/sensor misconfigured for this stretch.",
    ],
    "Water Supply": [
        "Leak-prone joint or low-pressure zone; repeated pipe choking in this lane.",
        "Valve leakage upstream causes recurring shortage at this point.",
    ],
    "Road Damage": [
        "Water stagnation + heavy vehicle load re-opens the same pothole.",
        "Weak sub-base; patch without resurfacing fails again within weeks.",
    ],
    "Garbage": [
        "No fixed pickup point here, so waste piles up again after clearing.",
        "Nearby shop/market waste dumped at same spot daily.",
    ],
    "Drainage": [
        "Silt/choke point downstream; same drain overflows after every rain.",
        "Broken slab cover lets silt in, blocking flow repeatedly.",
    ],
    "Manhole": [
        "Loose / broken cover reseats poorly and rattles open again under traffic.",
        "Sunken frame + weak surround lets the same cover sink after each repair.",
        "Gas / silt pressure pops the same lid after every rain.",
    ],
    "Other": [
        "Same unresolved root cause keeps retriggering complaints here.",
    ],
}

AI_FIX = {
    "Street Light": "Check pole wiring + voltage, replace fitting (not just bulb), seal cap, verify timer/sensor.",
    "Water Supply": "Inspect upstream valve + joints; flush line and fix leak point permanently.",
    "Road Damage": "Full patch with compaction + resurfacing, fix drainage alongside.",
    "Garbage": "Fix a marked bin point + daily pickup slot for this spot.",
    "Drainage": "Desilt full stretch + repair cover/slope so flow clears.",
    "Manhole": "Reset frame with concrete surround, fit heavy-duty cover, check level after compaction.",
    "Other": "Field visit to find the repeating trigger and fix permanently.",
}

def ai_repeat_insight(db, complaint, radius_km=0.5):
    """Return dict: same_area_count, nearby_count, total_repeat, why, fix, hotspot(bool)."""
    cat = complaint["category"]
    area_norm = normalize_area(complaint["area"])
    cid = complaint["id"]
    rows = db.execute("SELECT id, area, latitude, longitude, category FROM complaints WHERE category=?", (cat,)).fetchall()
    same_area = 0
    nearby = 0
    for r in rows:
        if r["id"] == cid:
            continue
        if normalize_area(r["area"]) == area_norm:
            same_area += 1
        if complaint["latitude"] and complaint["longitude"] and r["latitude"] and r["longitude"]:
            d = haversine_km(complaint["latitude"], complaint["longitude"], r["latitude"], r["longitude"])
            if d is not None and d <= radius_km:
                nearby += 1
    total = same_area + nearby
    # nearby-only complaints double counted if both same area+nearby; dedupe approx:
    total = max(same_area, nearby) if (same_area and nearby) else same_area + nearby
    causes = AI_CAUSES.get(cat, AI_CAUSES["Other"])
    idx = min(total, len(causes) - 1) if total else 0
    hotspot = total >= 2  # 3rd complaint onward = hotspot
    if total == 0:
        why = "First report from this spot. No repeat pattern yet."
    elif total == 1:
        why = f"2nd similar report here. Early repeat signal — {causes[0]}"
    else:
        why = f"{total+1} similar reports from same/nearby spot. Likely repeating cause: {causes[idx]}"
    return {"same_area": same_area, "nearby": nearby, "total_repeat": total,
            "why": why, "fix": AI_FIX.get(cat, AI_FIX["Other"]), "hotspot": hotspot}


def ai_hotspots(db, min_count=2, radius_km=0.5):
    """Group by (category, normalized area) + GPS clustering approx. Return list of hotspots."""
    rows = db.execute("SELECT * FROM complaints ORDER BY id DESC").fetchall()
    groups = {}
    for r in rows:
        key = (r["category"], normalize_area(r["area"]))
        groups.setdefault(key, []).append(r)
    spots = []
    for (cat, anorm), items in groups.items():
        if len(items) >= min_count:
            causes = AI_CAUSES.get(cat, AI_CAUSES["Other"])
            idx = min(len(items)-1, len(causes)-1)
            spots.append({"category": cat, "area": items[0]["area"], "count": len(items),
                          "ids": [x["id"] for x in items[:5]],
                          "why": causes[idx], "fix": AI_FIX.get(cat, AI_FIX["Other"]),
                          "latest": items[0]["created_at"]})
    spots.sort(key=lambda x: x["count"], reverse=True)
    return spots


def login_required(f):
    @wraps(f)
    def wrapper(*args, **kwargs):
        if "user_id" not in session:
            flash("Please login first.", "warning")
            return redirect(url_for("login"))
        return f(*args, **kwargs)
    return wrapper


def role_required(role):
    def decorator(f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            if session.get("role") != role:
                flash("Access denied.", "danger")
                return redirect(url_for("dashboard"))
            return f(*args, **kwargs)
        return wrapper
    return decorator


@app.route("/")
def home():
    db = get_db()
    try:
        total = db.execute("SELECT COUNT(*) as c FROM complaints").fetchone()["c"]
        pending = db.execute("SELECT COUNT(*) as c FROM complaints WHERE status='Pending'").fetchone()["c"]
        inprog = db.execute("SELECT COUNT(*) as c FROM complaints WHERE status='In Progress'").fetchone()["c"]
        done = db.execute("SELECT COUNT(*) as c FROM complaints WHERE status='Completed'").fetchone()["c"]
        recent = db.execute(
            "SELECT c.*, u.name as citizen_name FROM complaints c JOIN users u ON c.citizen_id=u.id ORDER BY c.id DESC LIMIT 3"
        ).fetchall()
    except Exception:
        total = pending = inprog = done = 0
        recent = []
    # Logged-in users still see hero but with dashboard link; don't force redirect
    # so the Alderway hero stays visible as landing.
    return render_template("home.html", total=total, pending=pending,
                           inprog=inprog, done=done, recent=recent)


@app.route("/register", methods=["GET", "POST"])
def register():
    if request.method == "POST":
        name = request.form.get("name", "").strip()
        email = request.form.get("email", "").strip().lower()
        password = request.form.get("password", "")
        role = request.form.get("role", "citizen")

        if role not in ("citizen", "engineer"):
            role = "citizen"
        if role == "engineer" and not ALLOW_ENGINEER_SIGNUP:
            flash("Engineer accounts are created by the CivicFix administrator. Please register as a citizen.", "danger")
            return render_template("register.html")
        if not name or not email or not password:
            flash("All fields are required.", "danger")
            return render_template("register.html")
        if len(password) < 10:
            flash("Use a password with at least 10 characters.", "danger")
            return render_template("register.html")

        db = get_db()
        try:
            db.execute(
                "INSERT INTO users (name, email, password, role, created_at) VALUES (?,?,?,?,?)",
                (name, email, generate_password_hash(password), role, datetime.now().isoformat(timespec="seconds")),
            )
            db.commit()
            flash("Registered successfully! Please login.", "success")
            return redirect(url_for("login"))
        except sqlite3.IntegrityError:
            flash("Email already registered.", "danger")

    return render_template("register.html")


@app.route("/login", methods=["GET", "POST"])
def login():
    if request.method == "POST":
        email = request.form.get("email", "").strip().lower()
        password = request.form.get("password", "")
        db = get_db()
        user = db.execute("SELECT * FROM users WHERE email=?", (email,)).fetchone()
        if user and check_password_hash(user["password"], password):
            session["user_id"] = user["id"]
            session["user_name"] = user["name"]
            session["role"] = user["role"]
            flash(f'Welcome, {user["name"]}!', "success")
            return redirect(url_for("dashboard"))
        flash("Invalid email or password.", "danger")
    return render_template("login.html")


@app.route("/logout")
def logout():
    session.clear()
    flash("Logged out.", "info")
    return redirect(url_for("home"))


@app.route("/dashboard")
@login_required
def dashboard():
    if session.get("role") == "engineer":
        return redirect(url_for("engineer_dashboard"))
    return redirect(url_for("citizen_dashboard"))


# ---------- CITIZEN ----------
@app.route("/citizen")
@login_required
@role_required("citizen")
def citizen_dashboard():
    db = get_db()
    status_filter = request.args.get("status", "All")
    if status_filter in STATUSES:
        complaints = db.execute(
            "SELECT * FROM complaints WHERE citizen_id=? AND status=? ORDER BY id DESC",
            (session["user_id"], status_filter),
        ).fetchall()
    else:
        complaints = db.execute(
            "SELECT * FROM complaints WHERE citizen_id=? ORDER BY id DESC",
            (session["user_id"],),
        ).fetchall()
        status_filter = "All"

    counts = {}
    for s in STATUSES:
        row = db.execute(
            "SELECT COUNT(*) as c FROM complaints WHERE citizen_id=? AND status=?",
            (session["user_id"], s),
        ).fetchone()
        counts[s] = row["c"]

    return render_template("citizen_dashboard.html", complaints=complaints,
                           counts=counts, status_filter=status_filter, statuses=STATUSES)


@app.route("/citizen/new", methods=["GET", "POST"])
@login_required
@role_required("citizen")
def new_complaint():
    if request.method == "POST":
        import uuid
        from werkzeug.utils import secure_filename
        category = request.form.get("category", "")
        title = request.form.get("title", "").strip()
        description = request.form.get("description", "").strip()
        area = request.form.get("area", "").strip()
        other_detail = request.form.get("other_detail", "").strip()
        lat_raw = request.form.get("latitude", "").strip()
        lng_raw = request.form.get("longitude", "").strip()

        if category not in CATEGORIES:
            flash("Please select a valid category.", "danger")
            return render_template("new_complaint.html", categories=CATEGORIES)
        if category == "Other":
            if not other_detail:
                flash("Please specify what the 'Other' issue is.", "danger")
                return render_template("new_complaint.html", categories=CATEGORIES)
            if len(other_detail) > 80:
                other_detail = other_detail[:80]
        else:
            other_detail = ""
        if not title or not description or not area:
            flash("Title, description and area are required.", "danger")
            return render_template("new_complaint.html", categories=CATEGORIES)

        try:
            latitude = float(lat_raw) if lat_raw else None
            longitude = float(lng_raw) if lng_raw else None
        except ValueError:
            latitude, longitude = None, None

        # Photo upload / take photo (mobile camera via capture attr in form)
        image_path = ""
        photo = request.files.get("photo")
        if photo and photo.filename:
            if not allowed_file(photo.filename):
                flash("Photo must be png/jpg/jpeg/gif/webp.", "danger")
                return render_template("new_complaint.html", categories=CATEGORIES)
            ext = photo.filename.rsplit(".", 1)[1].lower()
            fname = f"{uuid.uuid4().hex}.{ext}"
            photo.save(os.path.join(app.config["UPLOAD_FOLDER"], secure_filename(fname)))
            image_path = f"uploads/{fname}"

        now = datetime.now().isoformat(timespec="seconds")
        db = get_db()
        db.execute(
            """INSERT INTO complaints
               (citizen_id, category, title, description, area, status, remark, image_path, latitude, longitude, other_detail, created_at, updated_at)
               VALUES (?,?,?,?,?,'Pending','',?,?,?, ?, ?,?)""",
            (session["user_id"], category, title, description, area, image_path, latitude, longitude, other_detail, now, now),
        )
        db.commit()
        flash("Complaint submitted successfully with photo & location!", "success")
        return redirect(url_for("citizen_dashboard"))

    return render_template("new_complaint.html", categories=CATEGORIES)


@app.route("/citizen/complaint/<int:cid>")
@login_required
@role_required("citizen")
def citizen_view_complaint(cid):
    db = get_db()
    c = db.execute(
        "SELECT c.*, u.name as citizen_name FROM complaints c JOIN users u ON c.citizen_id=u.id WHERE c.id=? AND c.citizen_id=?",
        (cid, session["user_id"]),
    ).fetchone()
    if not c:
        flash("Complaint not found.", "danger")
        return redirect(url_for("citizen_dashboard"))
    ai = ai_repeat_insight(db, c)
    return render_template("view_complaint.html", c=c, is_engineer=False, ai=ai)


# ---------- ENGINEER ----------
@app.route("/engineer")
@login_required
@role_required("engineer")
def engineer_dashboard():
    db = get_db()
    status_filter = request.args.get("status", "All")
    category_filter = request.args.get("category", "All")

    query = """SELECT c.*, u.name as citizen_name FROM complaints c
               JOIN users u ON c.citizen_id=u.id WHERE 1=1"""
    params = []
    if status_filter in STATUSES:
        query += " AND c.status=?"
        params.append(status_filter)
    if category_filter in CATEGORIES:
        query += " AND c.category=?"
        params.append(category_filter)
    query += " ORDER BY c.id DESC"

    complaints = db.execute(query, params).fetchall()

    counts = {}
    for s in STATUSES:
        row = db.execute("SELECT COUNT(*) as c FROM complaints WHERE status=?", (s,)).fetchone()
        counts[s] = row["c"]
    total = db.execute("SELECT COUNT(*) as c FROM complaints").fetchone()["c"]
    auto_count = db.execute("SELECT COUNT(*) as c FROM complaints WHERE source='esp32'").fetchone()["c"]
    max_auto_id = db.execute("SELECT COALESCE(MAX(id), 0) AS max_id FROM complaints WHERE source='esp32'").fetchone()["max_id"]

    return render_template("engineer_dashboard.html", complaints=complaints,
                           counts=counts, total=total, auto_count=auto_count,
                           max_auto_id=max_auto_id,
                           status_filter=status_filter, category_filter=category_filter,
                           statuses=STATUSES, categories=CATEGORIES)


@app.route("/engineer/complaint/<int:cid>", methods=["GET", "POST"])
@login_required
@role_required("engineer")
def engineer_view_complaint(cid):
    db = get_db()
    if request.method == "POST":
        status = request.form.get("status", "")
        remark = request.form.get("remark", "").strip()
        if status not in STATUSES:
            flash("Invalid status.", "danger")
        else:
            db.execute(
                "UPDATE complaints SET status=?, remark=?, updated_at=? WHERE id=?",
                (status, remark, datetime.now().isoformat(timespec="seconds"), cid),
            )
            db.commit()
            flash("Complaint updated successfully!", "success")
        return redirect(url_for("engineer_view_complaint", cid=cid))

    c = db.execute(
        "SELECT c.*, u.name as citizen_name, u.email as citizen_email FROM complaints c JOIN users u ON c.citizen_id=u.id WHERE c.id=?",
        (cid,),
    ).fetchone()
    if not c:
        flash("Complaint not found.", "danger")
        return redirect(url_for("engineer_dashboard"))
    ai = ai_repeat_insight(db, c)
    return render_template("view_complaint.html", c=c, is_engineer=True, statuses=STATUSES, ai=ai)


@app.route("/engineer/insights")
@login_required
@role_required("engineer")
def engineer_insights():
    db = get_db()
    spots = ai_hotspots(db, min_count=2)
    return render_template("insights.html", spots=spots)


# ---------- ESP32 DEVICES (engineer only) ----------
def clean_esp_ip(raw):
    """Accept '192.168.1.50', '192.168.1.50:80', or full URL. Return host[:port] or None."""
    import re
    if not raw:
        return None
    s = raw.strip()
    s = re.sub(r"^https?://", "", s, flags=re.I)
    s = s.split("/")[0].strip()
    # IPv4 + optional port, or mDNS/hostname + optional port
    if re.match(r"^(\d{1,3}\.){3}\d{1,3}(:\d{1,5})?$", s):
        parts = s.split(":")[0].split(".")
        if all(0 <= int(p) <= 255 for p in parts):
            return s
        return None
    if re.match(r"^[a-zA-Z0-9_.-]+(:\d{1,5})?$", s):
        return s
    return None


@app.route("/engineer/devices", methods=["GET", "POST"])
@login_required
@role_required("engineer")
def esp_devices():
    db = get_db()
    if request.method == "POST":
        name = request.form.get("name", "").strip() or "Street Light Controller"
        ip_raw = request.form.get("ip", "")
        area = request.form.get("area", "").strip()
        ip = clean_esp_ip(ip_raw)
        if not ip:
            flash("Enter a valid ESP32 IP, e.g. 192.168.1.50 or 192.168.1.50:80", "danger")
        else:
            db.execute(
                "INSERT INTO esp_devices (name, ip, area, added_by, created_at) VALUES (?,?,?,?,?)",
                (name, ip, area, session["user_id"], datetime.now().isoformat(timespec="seconds")),
            )
            db.commit()
            flash(f"ESP32 {ip} saved! Click Open to go to its webserver.", "success")
        return redirect(url_for("esp_devices"))
    devices = db.execute("SELECT * FROM esp_devices ORDER BY id DESC").fetchall()
    return render_template("esp_devices.html", devices=devices)


@app.route("/engineer/devices/<int:did>/delete", methods=["POST"])
@login_required
@role_required("engineer")
def esp_device_delete(did):
    db = get_db()
    db.execute("DELETE FROM esp_devices WHERE id=?", (did,))
    db.commit()
    flash("Device removed.", "info")
    return redirect(url_for("esp_devices"))


@app.route("/engineer/devices/<int:did>/open")
@login_required
@role_required("engineer")
def esp_device_open(did):
    """Directly goes to the ESP32 webserver."""
    db = get_db()
    d = db.execute("SELECT * FROM esp_devices WHERE id=?", (did,)).fetchone()
    if not d:
        flash("Device not found.", "danger")
        return redirect(url_for("esp_devices"))
    return redirect(f"http://{d['ip']}/")


@app.route("/api/esp32/telemetry", methods=["POST"])
def esp32_telemetry():
    """Receive authenticated ESP32 readings and create one complaint per fault episode."""
    import json
    expected = os.environ.get("ESP32_DEVICE_TOKEN", "").strip()
    supplied = request.headers.get("X-Device-Token", "").strip()
    if not expected:
        return jsonify(ok=False, error="ESP32_DEVICE_TOKEN is not configured on the server"), 503
    if not supplied or not __import__("hmac").compare_digest(supplied, expected):
        return jsonify(ok=False, error="Unauthorized device"), 401
    payload = request.get_json(silent=True) or {}
    device_name = str(payload.get("device_name", "ESP32 Infrastructure Node"))[:100].strip() or "ESP32 Infrastructure Node"
    area = str(payload.get("address", ""))[:200].strip() or "Site address not configured"
    try:
        latitude = float(payload.get("latitude"))
        longitude = float(payload.get("longitude"))
        if not (-90 <= latitude <= 90 and -180 <= longitude <= 180) or (latitude == 0 and longitude == 0):
            latitude = longitude = None
    except (TypeError, ValueError):
        latitude = longitude = None
    readings = payload.get("readings", {})
    if not isinstance(readings, dict):
        readings = {}
    alerts = payload.get("alerts", [])
    if not isinstance(alerts, list):
        return jsonify(ok=False, error="alerts must be a list"), 400
    allowed = {
        "street_light_broken": ("Street Light", "Street light not working", "Street light is expected to be ON at night, but LDR #1 did not detect illumination."),
        "manhole_overflow_risk": ("Manhole", "Manhole level at overflow risk", "The ultrasonic sensor reports that the manhole fill level has reached the danger threshold."),
        "drainage_pipe_full": ("Drainage", "Drainage pipe is full", "The drainage sensor reports a full or blocked pipe."),
        "pipeline_leakage": ("Water Supply", "Pipeline leakage detected", "The two flow sensors report a flow difference above the configured threshold."),
    }
    db = get_db()
    now = datetime.now().isoformat(timespec="seconds")
    user = db.execute("SELECT id FROM users WHERE email=?", ("auto-alerts@civicfix.local",)).fetchone()
    if not user:
        cur = db.execute("INSERT INTO users (name,email,password,role,created_at) VALUES (?,?,?,?,?)",
                         ("ESP32 Automatic Alerts", "auto-alerts@civicfix.local", generate_password_hash(os.urandom(24).hex()), "citizen", now))
        system_user_id = cur.lastrowid
    else:
        system_user_id = user["id"]
    created = []
    for alert in alerts:
        if not isinstance(alert, dict):
            continue
        key = str(alert.get("key", ""))
        if key not in allowed:
            continue
        active = bool(alert.get("active", False))
        state = db.execute("SELECT * FROM esp_alert_state WHERE device_name=? AND trigger_key=?", (device_name, key)).fetchone()
        if not state:
            db.execute("INSERT INTO esp_alert_state (device_name,trigger_key,active,last_seen) VALUES (?,?,?,?)", (device_name,key,1 if active else 0,now))
            was_active = False
        else:
            was_active = bool(state["active"])
            db.execute("UPDATE esp_alert_state SET active=?, last_seen=? WHERE id=?", (1 if active else 0,now,state["id"]))
        if not active:
            continue
        # Don't create a new ticket on every telemetry poll; reuse an unresolved ticket.
        open_ticket = db.execute("SELECT id FROM complaints WHERE source='esp32' AND device_name=? AND trigger_key=? AND status!='Completed' ORDER BY id DESC LIMIT 1", (device_name,key)).fetchone()
        if open_ticket:
            if state:
                db.execute("UPDATE esp_alert_state SET last_complaint_id=? WHERE device_name=? AND trigger_key=?", (open_ticket["id"],device_name,key))
            continue
        # If this fault stayed active while an engineer closed its ticket, do not reopen it
        # on every poll. Require a clear reading followed by a new active transition.
        if was_active:
            continue
        category, title, default_desc = allowed[key]
        desc = str(alert.get("description", default_desc))[:2000]
        reading_text = json.dumps({"readings": readings, "alert": alert}, ensure_ascii=False)[:6000]
        cur = db.execute("""INSERT INTO complaints
            (citizen_id,category,title,description,area,status,remark,image_path,latitude,longitude,other_detail,source,device_name,trigger_key,sensor_data,created_at,updated_at)
            VALUES (?,?,?,?,?,'Pending','', '', ?, ?, '', 'esp32', ?, ?, ?, ?, ?)""",
            (system_user_id, category, title, desc, area, latitude, longitude, device_name, key, reading_text, now, now))
        cid = cur.lastrowid
        db.execute("UPDATE esp_alert_state SET last_complaint_id=? WHERE device_name=? AND trigger_key=?", (cid,device_name,key))
        created.append({"id": cid, "key": key, "category": category, "title": title, "message": f"{title} complaint registered successfully."})
    db.commit()
    return jsonify(ok=True, device=device_name, created=created, received_at=now), 200


@app.route("/api/engineer/auto-complaints")
@login_required
@role_required("engineer")
def engineer_auto_complaints():
    """Return newly created ESP32 complaints for live dashboard notifications."""
    try:
        after_id = max(0, int(request.args.get("after_id", "0")))
    except (TypeError, ValueError):
        after_id = 0
    db = get_db()
    rows = db.execute(
        """SELECT id, category, title, area, status, created_at
           FROM complaints
           WHERE source='esp32' AND id > ?
           ORDER BY id ASC LIMIT 20""",
        (after_id,),
    ).fetchall()
    items = [
        {
            "id": row["id"],
            "category": row["category"],
            "title": row["title"],
            "area": row["area"],
            "status": row["status"],
            "created_at": row["created_at"],
            "message": f"{row['title']} complaint registered successfully (#{row['id']}).",
        }
        for row in rows
    ]
    return jsonify(ok=True, complaints=items), 200


@app.route("/healthz")
def healthz():
    """UptimeRobot pings this every 5 min to keep Render free tier awake.
    Returns 200 + JSON so monitors can check keyword 'ok'."""
    uptime = int(time.time() - START_TIME)
    try:
        db = sqlite3.connect(DATABASE, timeout=3)
        db.execute("SELECT 1").fetchone()
        db.close()
        db_status = "ok"
    except Exception:
        app.logger.exception("Health check database failure")
        db_status = "error"
    status_code = 200 if db_status == "ok" else 503
    return jsonify(status="ok" if db_status == "ok" else "error",
                   uptime_seconds=uptime, db=db_status), status_code


@app.route("/privacy")
def privacy():
    return render_template("privacy.html")


@app.route("/status")
def status():
    """Public status page (human-friendly) for UptimeRobot concept demo."""
    uptime = int(time.time() - START_TIME)
    hrs, rem = divmod(uptime, 3600)
    mins, secs = divmod(rem, 60)
    return render_template("status.html", uptime_seconds=uptime,
                           uptime_str=f"{hrs}h {mins}m {secs}s")


if __name__ == "__main__":
    init_db()
    port = int(os.environ.get("PORT", 5000))
    debug = os.environ.get("FLASK_DEBUG", "0") == "1"
    print(f"Database ready. Open http://0.0.0.0:{port}")
    app.run(debug=debug, host="0.0.0.0", port=port)
else:
    # Gunicorn / Render: ensure DB exists on worker boot
    try:
        init_db()
    except Exception as e:
        print(f"DB init skipped: {e}")
