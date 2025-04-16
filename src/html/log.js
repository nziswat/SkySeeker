// functions for loading aircraft from database and displaying relevant data in the log.



//debug functions

function debugDatabase(){
    window.cefQuery({
        request: "debugDatabase",
        onSuccess: (response) => {
            let x;
        },
        onFailure: function (error_code, error_message) {
            //console.error("Aircraft not found", error_code, error_message);
        }
    });
}

function debugDeleteDatabase() {
    window.cefQuery({
        request: "debugDeleteDatabase",
        onSuccess: (response) => {
            let x;
        },
        onFailure: function (error_code, error_message) {
            //console.error("Aircraft not found", error_code, error_message);
        }
    });
}