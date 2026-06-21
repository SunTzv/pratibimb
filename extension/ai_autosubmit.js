// ai_autosubmit.js
// Content script to intercept pending AI queries and auto-submit them

if (typeof chrome !== 'undefined' && chrome.storage && chrome.storage.local) {
    chrome.storage.local.get(['pending_ai_data'], (res) => {
        if (res.pending_ai_data) {
            const data = res.pending_ai_data;
            // Immediately clear to prevent it from firing again on reload
            chrome.storage.local.remove('pending_ai_data');
            
            // Poll until the chat interface's input box is ready and active
            let attempts = 0;
            const interval = setInterval(() => {
                attempts++;
                if (attempts > 30) { // Max 15 seconds
                    clearInterval(interval);
                    return;
                }
                
                // Domain-specific reliable selectors
                const domain = window.location.hostname;
                let inputEl = null;
                let sendBtnEl = null;
                
                if (domain.includes('chatgpt.com')) {
                    inputEl = document.querySelector('#prompt-textarea');
                    sendBtnEl = document.querySelector('[data-testid="send-button"]');
                } else if (domain.includes('claude.ai')) {
                    inputEl = document.querySelector('[contenteditable="true"]');
                    sendBtnEl = document.querySelector('button[aria-label="Send Message"]');
                } else if (domain.includes('gemini.google.com')) {
                    inputEl = document.querySelector('rich-textarea [contenteditable="true"]') || document.querySelector('.ql-editor');
                    // Gemini's send button usually has an aria-label containing "Send" or "Submit"
                    sendBtnEl = document.querySelector('button[aria-label*="Send message" i]') || document.querySelector('.send-button');
                } else if (domain.includes('grok.com') || domain.includes('x.com')) {
                    inputEl = document.querySelector('textarea');
                    const buttons = Array.from(document.querySelectorAll('button'));
                    sendBtnEl = buttons.find(b => b.textContent.toLowerCase().includes('grok') || (b.getAttribute('aria-label') && b.getAttribute('aria-label').toLowerCase().includes('send')));
                }
                
                // Fallback heuristic if specific selectors fail (due to UI updates)
                if (!inputEl) {
                    const inputs = document.querySelectorAll('textarea, [contenteditable="true"]');
                    if (inputs.length > 0) inputEl = inputs[inputs.length - 1];
                }
                
                const isInputReady = inputEl && (
                    inputEl.tagName === 'TEXTAREA' || 
                    inputEl.getAttribute('contenteditable') === 'true' || 
                    inputEl.isContentEditable
                );
                                      
                if (isInputReady) {
                    clearInterval(interval);
                    
                    if (data.action === 'paste_and_enter') {
                        inputEl.focus();
                        document.execCommand('insertText', false, data.text);
                    }
                }
            }, 500);
        }
    });
}
