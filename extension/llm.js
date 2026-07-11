// llm.js
// Content script to intercept pending AI queries and securely paste them (no auto-submit)

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
                
                const domain = window.location.hostname;
                let inputEl = null;
                
                if (domain.includes('gemini.google.com')) {
                    inputEl = document.querySelector('rich-textarea [contenteditable="true"]') || document.querySelector('.ql-editor');
                } else if (domain.includes('grok.com') || domain.includes('x.com')) {
                    inputEl = document.querySelector('textarea');
                } else if (domain.includes('claude.ai')) {
                    inputEl = document.querySelector('[contenteditable="true"]');
                }
                
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
                    
                    if (data.action === 'paste_only') {
                        inputEl.focus();
                        document.execCommand('insertText', false, data.text);
                        // We do NOT simulate Enter or click send buttons here anymore.
                        // The user will manually review and submit the prompt.
                    }
                }
            }, 500);
        }
    });
}
