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
                    
                    // We now universally use paste_and_enter to trigger UI states natively
                    if (data.action === 'paste_and_enter') {
                        inputEl.focus();
                        document.execCommand('insertText', false, data.text);
                    }
                    
                    // Poll for the exact send button, no fallback heuristics
                    let submitAttempts = 0;
                    const submitInterval = setInterval(() => {
                        submitAttempts++;
                        if (submitAttempts > 150) { // 15 second timeout to account for slow hydration
                            clearInterval(submitInterval);
                            return;
                        }
                        
                        // React/Next.js often completely replaces the DOM node during hydration!
                        // If our input element is detached, we MUST re-query it!
                        if (!inputEl || !document.body.contains(inputEl)) {
                            if (domain.includes('chatgpt.com')) {
                                inputEl = document.querySelector('#prompt-textarea');
                            } else if (domain.includes('claude.ai')) {
                                inputEl = document.querySelector('[contenteditable="true"]');
                            } else if (domain.includes('gemini.google.com')) {
                                inputEl = document.querySelector('rich-textarea [contenteditable="true"]') || document.querySelector('.ql-editor');
                            } else if (domain.includes('grok.com') || domain.includes('x.com')) {
                                inputEl = document.querySelector('textarea');
                            } else {
                                const inputs = document.querySelectorAll('textarea, [contenteditable="true"]');
                                if (inputs.length > 0) inputEl = inputs[inputs.length - 1];
                            }
                        }
                        
                        if (!inputEl) return;
                        
                        // Wait until the text box has content (either from our paste, or from their URL parameter)
                        const textContent = (inputEl.value || inputEl.textContent || '').trim();
                        if (!textContent) return;
                        
                        // We have our text! Clear the interval so we only submit once.
                        clearInterval(submitInterval);
                        
                        // Wait 300ms for React/Angular to finish hydrating the "text entered" state
                        setTimeout(() => {
                            inputEl.focus();
                            
                            // Simulate a native Enter key press
                            const enterOpts = { 
                                key: 'Enter', 
                                code: 'Enter', 
                                keyCode: 13, 
                                which: 13, 
                                bubbles: true, 
                                cancelable: true, 
                                composed: true 
                            };
                            inputEl.dispatchEvent(new KeyboardEvent('keydown', enterOpts));
                            inputEl.dispatchEvent(new KeyboardEvent('keypress', enterOpts));
                            inputEl.dispatchEvent(new KeyboardEvent('keyup', enterOpts));
                            
                            // Fallback: If the site explicitly blocks untrusted keyboard events, 
                            // bypass the key listener and trigger a native DOM form submission.
                            setTimeout(() => {
                                const form = inputEl.closest('form');
                                if (form) {
                                    try {
                                        form.requestSubmit();
                                    } catch (e) {
                                        form.dispatchEvent(new Event('submit', { cancelable: true, bubbles: true }));
                                    }
                                }
                            }, 200);
                            
                        }, 300);
                        
                    }, 100);
                }
            }, 500);
        }
    });
}
