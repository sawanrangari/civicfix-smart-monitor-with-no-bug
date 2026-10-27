(() => {
  const dictionaries = {
    hi: {
      'Home':'होम','Public status':'सार्वजनिक स्थिति','Sign in':'लॉग इन','Get started':'शुरू करें',
      'CivicFix · Street Light & Civic Complaints':'CivicFix · स्ट्रीट लाइट और नागरिक शिकायतें',
      'Built for clearer, more accessible civic services.':'नागरिक सेवाओं को सरल और सुलभ बनाने के लिए।',
      'Need help?':'मदद चाहिए?','CivicFix Help':'CivicFix सहायता','Citizen support · Available anytime':'नागरिक सहायता · कभी भी उपलब्ध',
      'Hi! I can help you submit a complaint, track its status, or understand the website. What do you need?':'नमस्ते! मैं शिकायत दर्ज करने, उसकी स्थिति देखने या वेबसाइट समझने में मदद कर सकता हूँ। आपको किस चीज़ में मदद चाहिए?',
      'Register a complaint':'शिकायत दर्ज करें','Track complaint':'शिकायत ट्रैक करें','Add location':'लोकेशन जोड़ें',
      'Quick guidance only. For emergencies, contact the relevant emergency service.':'यह सामान्य मार्गदर्शन है। आपात स्थिति में संबंधित आपातकालीन सेवा से संपर्क करें।',
      'What needs fixing?':'क्या ठीक करवाना है?','Snap a photo, capture live location, engineer sees it instantly.':'फोटो लें, लाइव लोकेशन जोड़ें और इंजीनियर तक जानकारी पहुँचाएँ।',
      'Category':'श्रेणी','Title':'शीर्षक','Area / Location (text)':'क्षेत्र / स्थान','Photo evidence (upload or take photo)':'फोटो प्रमाण (अपलोड करें या फोटो लें)',
      'Real-time location':'वर्तमान लोकेशन','Use my live location':'मेरी वर्तमान लोकेशन इस्तेमाल करें','Description':'विवरण',
      'Submit with photo & location':'फोटो और लोकेशन के साथ जमा करें','Pole no., landmark, since when...':'पोल नंबर, नज़दीकी जगह, समस्या कब से है...',
      'e.g. Street light not working near bus stop':'जैसे बस स्टॉप के पास स्ट्रीट लाइट बंद है','e.g. Sector 5, MG Road, Ward 12':'जैसे सेक्टर 5, MG रोड, वार्ड 12',
      'Report an issue':'समस्या दर्ज करें','Engineer portal':'इंजीनियर पोर्टल','A better way to':'अपने इलाके की समस्याएँ','fix your neighbourhood':'सुलझाने का बेहतर तरीका',
      'Report street lights, roads, drainage, water and other local issues. Follow progress from submission to resolution, all in one place.':'स्ट्रीट लाइट, सड़क, नाली, पानी और अन्य स्थानीय समस्याएँ दर्ज करें। शिकायत जमा करने से समाधान तक उसकी प्रगति देखें।',
      'Free to use · Clear status updates · Engineer remarks':'मुफ़्त सेवा · स्पष्ट स्थिति अपडेट · इंजीनियर की टिप्पणी','Small report. Real-world impact.':'छोटी शिकायत, असली बदलाव।',
      'Citizens raise issues, teams take action, and progress stays visible.':'नागरिक समस्या बताते हैं, टीम कार्रवाई करती है और प्रगति दिखाई देती है।','Get started':'शुरू करें'
    },
    mr: {
      'Home':'मुख्यपृष्ठ','Public status':'सार्वजनिक स्थिती','Sign in':'लॉग इन','Get started':'सुरू करा',
      'CivicFix · Street Light & Civic Complaints':'CivicFix · पथदिवे आणि नागरी तक्रारी',
      'Built for clearer, more accessible civic services.':'नागरी सेवा अधिक सोप्या आणि सुलभ करण्यासाठी.',
      'Need help?':'मदत हवी आहे?','CivicFix Help':'CivicFix मदत','Citizen support · Available anytime':'नागरिक सहाय्य · कधीही उपलब्ध',
      'Hi! I can help you submit a complaint, track its status, or understand the website. What do you need?':'नमस्कार! तक्रार नोंदवणे, तिची स्थिती पाहणे किंवा वेबसाइट समजून घेण्यासाठी मी मदत करू शकतो. तुम्हाला कोणती मदत हवी आहे?',
      'Register a complaint':'तक्रार नोंदवा','Track complaint':'तक्रारीचा मागोवा घ्या','Add location':'ठिकाण जोडा',
      'Quick guidance only. For emergencies, contact the relevant emergency service.':'ही प्राथमिक माहिती आहे. आपत्कालीन परिस्थितीत संबंधित आपत्कालीन सेवेशी संपर्क साधा.',
      'What needs fixing?':'काय दुरुस्त करायचे आहे?','Snap a photo, capture live location, engineer sees it instantly.':'फोटो जोडा, सध्याचे ठिकाण नोंदवा आणि अभियंत्यापर्यंत माहिती पोहोचवा.',
      'Category':'प्रकार','Title':'शीर्षक','Area / Location (text)':'परिसर / ठिकाण','Photo evidence (upload or take photo)':'फोटो पुरावा (अपलोड करा किंवा फोटो काढा)',
      'Real-time location':'सध्याचे ठिकाण','Use my live location':'माझे सध्याचे ठिकाण वापरा','Description':'तपशील',
      'Submit with photo & location':'फोटो आणि ठिकाणासह पाठवा','Pole no., landmark, since when...':'खांब क्रमांक, जवळची खूण, समस्या कधीपासून...',
      'e.g. Street light not working near bus stop':'उदा. बस थांब्याजवळील पथदिवा बंद आहे','e.g. Sector 5, MG Road, Ward 12':'उदा. सेक्टर ५, एमजी रोड, प्रभाग १२',
      'Report an issue':'समस्या नोंदवा','Engineer portal':'अभियंता पोर्टल','A better way to':'आपला परिसर सुधारण्याचा','fix your neighbourhood':'अधिक सोपा मार्ग',
      'Report street lights, roads, drainage, water and other local issues. Follow progress from submission to resolution, all in one place.':'पथदिवे, रस्ते, गटार, पाणी आणि इतर स्थानिक समस्या नोंदवा. तक्रार दिल्यापासून निराकरण होईपर्यंतची प्रगती एकाच ठिकाणी पाहा.',
      'Free to use · Clear status updates · Engineer remarks':'मोफत सेवा · स्पष्ट स्थिती अपडेट · अभियंत्याची नोंद','Small report. Real-world impact.':'छोटी तक्रार, प्रत्यक्ष बदल.',
      'Citizens raise issues, teams take action, and progress stays visible.':'नागरिक समस्या नोंदवतात, पथके कारवाई करतात आणि प्रगती दिसत राहते.','Get started':'सुरू करा'
    }
  };
  Object.assign(dictionaries.hi, {
    'Dashboard':'डैशबोर्ड','Sign out':'लॉग आउट','Pending':'लंबित','In Progress':'कार्य प्रगति पर','Completed':'पूरा हुआ',
    'Citizen':'नागरिक','Engineer':'इंजीनियर','Tracking':'ट्रैकिंग','Resolution score':'समाधान स्कोर','Total':'कुल',
    'What needs fixing?':'क्या ठीक करवाना है?','Welcome':'स्वागत है','back':'वापस','Email':'ईमेल','Password':'पासवर्ड',
    'Submit':'जमा करें','Road':'सड़क','Street Light':'स्ट्रीट लाइट','Water':'पानी','Garbage':'कचरा','Drainage':'नाली','Manhole':'मैनहोल','Other':'अन्य'
  });
  Object.assign(dictionaries.mr, {
    'Dashboard':'डॅशबोर्ड','Sign out':'लॉग आउट','Pending':'प्रलंबित','In Progress':'काम सुरू','Completed':'पूर्ण',
    'Citizen':'नागरिक','Engineer':'अभियंता','Tracking':'मागोवा','Resolution score':'निराकरण गुण','Total':'एकूण',
    'What needs fixing?':'काय दुरुस्त करायचे आहे?','Welcome':'स्वागत आहे','back':'पुन्हा स्वागत','Email':'ईमेल','Password':'पासवर्ड',
    'Submit':'पाठवा','Road':'रस्ता','Street Light':'पथदिवा','Water':'पाणी','Garbage':'कचरा','Drainage':'गटार','Manhole':'मॅनहोल','Other':'इतर'
  });

  const languageSelect = document.getElementById('siteLanguage');
  let currentLanguage = localStorage.getItem('civicfix-language') || 'en';
  const originalText = new WeakMap();
  const originalAttrs = new WeakMap();
  const isTextNode = n => n.nodeType === Node.TEXT_NODE && n.nodeValue.trim();

  function translatePage(lang) {
    currentLanguage = lang;
    if (languageSelect) languageSelect.value = lang;
    const dict = dictionaries[lang] || {};
    document.documentElement.lang = lang === 'hi' ? 'hi' : lang === 'mr' ? 'mr' : 'en';
    document.querySelectorAll('[data-i18n]').forEach(el => {
      if (!originalText.has(el)) originalText.set(el, el.textContent);
      const source = el.getAttribute('data-i18n');
      el.textContent = lang === 'en' ? source : (dict[source] || source);
    });
    document.querySelectorAll('[data-i18n-placeholder]').forEach(el => {
      if (!originalAttrs.has(el)) originalAttrs.set(el, el.getAttribute('placeholder'));
      const source = el.getAttribute('data-i18n-placeholder');
      el.setAttribute('placeholder', lang === 'en' ? source : (dict[source] || source));
    });
    // Translate standalone interface text without touching dynamic complaint data or form values.
    const walker = document.createTreeWalker(document.body, NodeFilter.SHOW_TEXT);
    const nodes=[]; while(walker.nextNode()) nodes.push(walker.currentNode);
    nodes.forEach(node => {
      if (!isTextNode(node) || node.parentElement.closest('script,style,textarea,input,option,[data-i18n]')) return;
      if (!originalText.has(node)) originalText.set(node, node.nodeValue);
      const source = originalText.get(node).trim();
      if (lang === 'en') node.nodeValue = originalText.get(node);
      else if (dict[source]) node.nodeValue = node.nodeValue.replace(source, dict[source]);
    });
    document.querySelectorAll('select option').forEach(option => {
      if (!originalText.has(option)) originalText.set(option, option.textContent);
      const source = originalText.get(option);
      if (lang === 'en') option.textContent = source;
      else option.textContent = dict[source] || source;
    });
    localStorage.setItem('civicfix-language', lang);
    updateChatGreeting();
  }
  if (languageSelect) {
    languageSelect.addEventListener('change', e => translatePage(e.target.value));
  }

  const chat = document.getElementById('helpChat');
  const launcher = document.getElementById('chatLauncher');
  const messages = document.getElementById('chatMessages');
  const input = document.getElementById('chatInput');
  const chatForm = document.getElementById('chatForm');
  const greeting = {
    en:'Hi! I can help you submit a complaint, track its status, or understand the website. What do you need?',
    hi:'नमस्ते! मैं शिकायत दर्ज करने, उसकी स्थिति देखने या वेबसाइट समझने में मदद कर सकता हूँ। आपको किस चीज़ में मदद चाहिए?',
    mr:'नमस्कार! तक्रार नोंदवणे, तिची स्थिती पाहणे किंवा वेबसाइट समजून घेण्यासाठी मी मदत करू शकतो. तुम्हाला कोणती मदत हवी आहे?'
  };
  const answers = {
    submit:{en:'To register a complaint: sign in or create a citizen account, open “Report an issue”, choose a category, add a title and area, describe the problem, then submit. A photo and GPS location can help the response team.',hi:'शिकायत दर्ज करने के लिए: नागरिक खाते में लॉग इन करें या नया खाता बनाएँ, “समस्या दर्ज करें” खोलें, श्रेणी चुनें, शीर्षक और क्षेत्र भरें, समस्या का विवरण लिखें और जमा करें। फोटो और GPS लोकेशन से टीम को मदद मिल सकती है।',mr:'तक्रार नोंदवण्यासाठी: नागरिक खात्यात लॉग इन करा किंवा नवीन खाते तयार करा, “समस्या नोंदवा” उघडा, प्रकार निवडा, शीर्षक व परिसर भरा, समस्येचा तपशील लिहा आणि पाठवा. फोटो आणि GPS ठिकाणामुळे पथकाला मदत होऊ शकते.'},
    track:{en:'Sign in with your citizen account and open Dashboard to view your submitted complaints and their latest status. “Pending” means waiting for action; “In Progress” means work has started; “Completed” means it was marked resolved.',hi:'अपने नागरिक खाते में लॉग इन करें और Dashboard खोलें। वहाँ आपकी शिकायतें और उनकी वर्तमान स्थिति दिखेगी। “Pending” का मतलब कार्रवाई बाकी है, “In Progress” का मतलब काम शुरू है, और “Completed” का मतलब समाधान दर्ज किया गया है।',mr:'नागरिक खात्यात लॉग इन करून Dashboard उघडा. तेथे तुमच्या तक्रारी आणि त्यांची सद्यस्थिती दिसेल. “Pending” म्हणजे कारवाई बाकी, “In Progress” म्हणजे काम सुरू, आणि “Completed” म्हणजे निराकरण नोंदवले आहे.'},
    location:{en:'On the new complaint form, tap “Use my live location” and allow your browser to access location. If you prefer not to share GPS, enter the area or landmark in the location field instead.',hi:'नई शिकायत के फॉर्म में “मेरी वर्तमान लोकेशन इस्तेमाल करें” दबाएँ और ब्राउज़र को लोकेशन की अनुमति दें। GPS साझा नहीं करना चाहते तो क्षेत्र या नज़दीकी पहचान स्थान लिख दें।',mr:'नवीन तक्रार फॉर्ममध्ये “माझे सध्याचे ठिकाण वापरा” दाबा आणि ब्राउझरला परवानगी द्या. GPS द्यायचा नसेल तर परिसर किंवा जवळची खूण लिहा.'},
    status:{en:'Complaint statuses are shown in your Dashboard. If a status has not changed, the responsible team may not have updated it yet. Include the complaint number when asking an engineer for an update.',hi:'शिकायत की स्थिति Dashboard में दिखती है। स्थिति नहीं बदली है तो संभव है कि संबंधित टीम ने अभी अपडेट न किया हो। अपडेट पूछते समय शिकायत नंबर दें।',mr:'तक्रारीची स्थिती Dashboard मध्ये दिसते. स्थिती बदलली नसेल तर संबंधित पथकाने अद्ययावत केले नसेल. माहिती विचारताना तक्रार क्रमांक द्या.'},
    account:{en:'Use Sign up to create a citizen account, then sign in with the same email and password. Keep your password private.',hi:'नागरिक खाता बनाने के लिए Sign up चुनें, फिर उसी ईमेल और पासवर्ड से लॉग इन करें। पासवर्ड किसी के साथ साझा न करें।',mr:'नागरिक खाते तयार करण्यासाठी Sign up निवडा आणि त्याच ईमेल व पासवर्डने लॉग इन करा. पासवर्ड कोणालाही सांगू नका.'},
    fallback:{en:'I can help with registering a complaint, tracking status, adding GPS location, or creating an account. Try one of the quick buttons below, or tell me a little more.',hi:'मैं शिकायत दर्ज करने, स्थिति देखने, GPS लोकेशन जोड़ने या खाता बनाने में मदद कर सकता हूँ। नीचे दिए गए बटन चुनें या अपना सवाल थोड़ा और बताएँ।',mr:'तक्रार नोंदवणे, स्थिती पाहणे, GPS ठिकाण जोडणे किंवा खाते तयार करणे यासाठी मी मदत करू शकतो. खालील बटण निवडा किंवा प्रश्न थोडा स्पष्ट करा.'}
  };
  function updateChatGreeting(){ if(messages && messages.children.length===1 && messages.firstElementChild.dataset.greeting==='yes') messages.firstElementChild.textContent=greeting[currentLanguage] || greeting.en; }
  if(messages && messages.firstElementChild) messages.firstElementChild.dataset.greeting='yes';
  function addMessage(text, who){ const div=document.createElement('div'); div.className='chat-msg '+who; div.textContent=text; messages.appendChild(div); messages.scrollTop=messages.scrollHeight; }
  function identify(text){
    const s=text.toLowerCase();
    if(/submit|register|new complaint|report|complaint|शिकायत|दर्ज|तक्रार|नोंदव|तक्रार/.test(s)) return 'submit';
    if(/track|status|progress|pending|dashboard|स्थिति|स्टेटस|स्थिती|मागोवा/.test(s)) return 'track';
    if(/location|gps|map|लोकेशन|जगह|स्थान|ठिकाण|पत्ता/.test(s)) return 'location';
    if(/login|sign in|sign up|account|password|खाता|लॉग|पासवर्ड|खाते/.test(s)) return 'account';
    if(/status|स्थिति|स्थिती/.test(s)) return 'status';
    return 'fallback';
  }
  function answer(intent){
    if(intent==='submit' && !document.querySelector('a[href*="new"]')) return answers.submit[currentLanguage] || answers.submit.en;
    return (answers[intent] || answers.fallback)[currentLanguage] || (answers[intent] || answers.fallback).en;
  }
  function handleQuestion(text){ if(!text.trim()) return; addMessage(text,'user'); const intent=identify(text); addMessage(answer(intent),'bot'); }
  document.getElementById('chatOpen')?.addEventListener('click',()=>{chat.hidden=false;launcher.style.display='none';input?.focus();});
  document.getElementById('chatClose')?.addEventListener('click',()=>{chat.hidden=true;launcher.style.display='block';});
  document.querySelectorAll('[data-chat-intent]').forEach(btn=>btn.addEventListener('click',()=>handleQuestion(btn.dataset.chatIntent)));
  chatForm?.addEventListener('submit',e=>{e.preventDefault();handleQuestion(input.value);input.value='';input.focus();});
  translatePage(currentLanguage);
})();
