✅ Correct way to return boolean from C to JS
✔ Return true (success)
webview_return(w, id, 0, "true"); // JS Promise resolves with `true`

✖ Return false (failure, but not an error)
webview_return(w, id, 0, "false"); // JS Promise resolves with `false`

❗ Return an actual error (causes JS .catch() to trigger)
webview_return(w, id, 1, "\"Something went wrong\""); // Must be JSON string

🧠 JavaScript-side behavior
JS Example:
window.myFn().then(success => {
    if (success) {
        console.log("It worked!");
    } else {
        console.log("It ran but failed.");
    }
}).catch(err => {
    console.error("Critical error:", err);
});