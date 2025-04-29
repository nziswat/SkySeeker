// functions for loading aircraft from database and displaying relevant data in the log.

function loadDatabase(callback) {
    window.cefQuery({
        request: "loadDatabase",
        onSuccess: (response) => {
            let parse = JSON.parse(response);
            console.log(parse)
            callback(parse);
        },
        onFailure: function (error_code, error_message) {
            console.log("yeah failed lol")
        }
    });
}
// function to check ICAO code and get extended info
function ICAOlog(icaoCheck) {
    return new Promise((resolve, reject) => {
        let query_string = `getICAOData${icaoCheck}`;
        window.cefQuery({
            request: query_string,
            onSuccess: (response) => {
                let parse = JSON.parse(response);
                let extendedInfo = (parse.typeCode + "_" + parse.country + "_" + parse.isMilitary);
                resolve(extendedInfo);  // properly return the value
            },
            onFailure: (error_code, error_message) => {
                console.error("Failed to check ICAO");
                reject("errored");      //  properly handle failure
            }
        });
    });
}



// TWO IDEAS, format database into similar way of mockDatabase or just access it directly (probably doing this)


// Fills the log with all of the saved planes

// Note: do the cards have to be created every time this page is loaded or is there a way we could save the already created ones?
// should the header be the model or icao ? currently model

// DEMO VERSION FOR NOW IDK HOW TO ACCESS DATABASE
// Mock data representing the database

// Function to create cards
async function createCard(aircraft) {
    try {
        const ICAOdata = await ICAOlog(aircraft.icao); // get ICAO data from the TSV
        console.log(`got data for ${aircraft.icao} is as follows ${ICAOdata}`)
        const ICAOarray = ICAOdata.split('_');
        const type = ICAOarray[0];
        const country = ICAOarray[1];
        const military = ICAOarray[2];

        const card = document.createElement("a");
        card.href = "logexp.html";
        card.className = "card";

        // Image, maybe get rid of, doing same image for all for now
        const img = document.createElement("img");
        img.src = "media/737max9.png";
        img.alt = `${aircraft.icao} Image`;
        img.className = "card-img";


        const textList = document.createElement("ul");
        textList.className = "card-text";

        const title = document.createElement("h");
        title.textContent = aircraft.model;
        textList.appendChild(title);

        // Ok this just adds empty space and kinda makes it look weird
        //textList.appendChild(document.createElement("br")); // idk why these are here tbh
        //textList.appendChild(document.createElement("br"));

        // Add the rest of the info to a list

        // ICAO

        const icaoItem = document.createElement("li");
        icaoItem.textContent = `ICAO: ${aircraft.icao}`;
        textList.appendChild(icaoItem);

        ICAOlog(aircraft.icao);


         // Type (aka Model)
         const typeItem = document.createElement("li");
         if (type != "UNKNOWN") {
             typeItem.textContent = `Model: ${type}`;
         }
         else {
             typeItem.textContent = `Model: Unknown`;
         }
         textList.appendChild(typeItem);

        // Date & Time setup
        let parts = aircraft.timestamp.split(" "); // Split the timestamp into date and time

        let date = parts[0]; // First part is the date
        let time = parts[1]; // Second part is the time


        // Date
        const dateItem = document.createElement("li");
        dateItem.textContent = `Date: ${date}`;
        textList.appendChild(dateItem);

        // Time
        const timeItem = document.createElement("li");
        timeItem.textContent = `Time: ${time}`;
        textList.appendChild(timeItem);

        // Latitude
        const latItem = document.createElement("li");
        latItem.textContent = `Lat:  ${aircraft.latitude}`;
        textList.appendChild(latItem);

        // Longitude
        const longItem = document.createElement("li");
        longItem.textContent = `Long: ${aircraft.longitude}`;
        textList.appendChild(longItem);

        // Country
        const countryItem = document.createElement("li");
        if (country != "UNKNOWN") {
            countryItem.textContent = `Country: ${country}`;
        }
        else {
            countryItem.textContent = `Country: Unknown`
        }
        textList.appendChild(countryItem);

        // Military
        const militaryItem = document.createElement("li");
        if (military != "UNKNOWN") {
            militaryItem.textContent = `Military: ${military}`;
        }
        else {
            militaryItem.textContent = `N/A`;
        }
        textList.appendChild(militaryItem);

        /*
        const { icao, model, date, time, lat, long, country, isMilitary } = aircraft;
        const content = { icao, date, time, lat, long, country, isMilitary }
        for (const [key, value] of Object.entries(content)) {
            const listItem = document.createElement("li");
            listItem.textContent = `${key}: ${value}`;
            textList.appendChild(listItem);
        }
        */
        // Add the image to the card
        card.appendChild(img);
        // Add the list to the card
        card.appendChild(textList);

        return card;
    }
    catch (error) {
        console.error(`Failed to get ICAO data for ${aircraft.icao}: ${error}`);

    }
}

// Function to render cards in rows of 3
async function renderCards(data) {
    const container = document.getElementById("cards-container");
    //container.innerHTML = ""; // Clear any existing cards // IDK what this does??
    let row;
    for (let index = 0; index < data.length; index++) {
        const aircraft = data[data.length - index - 1];
        if (index % 3 === 0) {
            row = document.createElement("div");
            row.className = "cards-container";
            container.appendChild(row);
        }
        const card = await createCard(aircraft);
        row.appendChild(card);
    };
}

loadDatabase(function(database) {
    console.log("Database loaded", database);
    renderCards(database);
});


//debug functions

function debugDatabase() {
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