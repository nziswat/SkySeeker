// functions for loading aircraft from database and displaying relevant data in the log.

function loadDatabase() {
    window.cefQuery({
        request: "loadDatabase",
        onSuccess: (response) => {
            let parse = JSON.parse(response);
            console.log(parse)
        },
        onFailure: function (error_code, error_message) {
            console.log("yeah failed lol")
        }
    });
}





// Fills the log with all of the saved planes

// Note: do the cards have to be created every time this page is loaded or is there a way we could save the already created ones?
// should the header be the model or icao ? currently model

// DEMO VERSION FOR NOW IDK HOW TO ACCESS DATABASE
// Mock data representing the database
const mockDatabase = [
    {
        model: "737",
        image: "media/model737.png", // we probably not doing images lowkey
        icao: "A40797",
        // Note: date & time is probably gonna have to be calculated in a more complicated way but this is demo example
        date: "4/24/2025",
        time: "21:15", // hey guys are we doing 24 hour time or 12 hour time ?
        lat: "27.48",
        long: "-82.34",
        country: "United States",
        isMilitary: "false",
    },
    {
        model: "1337",
        image: "media/model1337.png",
        icao: "A1738",
        date: "4/24/2025",
        time: "21:20",
        lat: "27.39",
        long: "-82.21",
        country: "United States",
        isMilitary: "true",
    },
    {   // why did copilot autogenerate this, (okay i just let it autogen like most of the rest neat)
        model: "C-130",
        image: "media/modelC130.png",
        icao: "A12345",
        date: "4/24/2025",
        time: "21:25",
        lat: "27.30",
        long: "-82.10",
        country: "United States",
        isMilitary: "true",
    },
    {   // Second group of 3
        model: "A380",
        image: "media/modelA380.png",
        icao: "A67890",
        date: "4/24/2025",
        time: "21:30",
        lat: "27.20",
        long: "-82.00",
        country: "United States",
        isMilitary: "false",
    },
    {
        model: "Boeing 747",
        image: "media/model747.png",
        icao: "A54321",
        date: "4/24/2025",
        time: "21:35",
        lat: "27.10",
        long: "-81.90",
        country: "United States",
        isMilitary: "false",
    },
    {
        model: "A320",
        image: "media/modelA320.png",
        icao: "A98765",
        date: "4/24/2025",
        time: "21:40",
        lat: "27.00",
        long: "-81.80",
        country: "United States",
        isMilitary: "false",
    },
    {   // Third group of 3 except its only 2 to test formatting
        model: "777",
        image: "media/model777.png",
        icao: "A13579",
        date: "4/24/2025",
        time: "21:45",
        lat: "26.90",
        long: "-81.70",
        country: "United States",
        isMilitary: "false",
    },
    {
        model: "787",
        image: "media/model787.png",
        icao: "A86420",
        date: "4/24/2025",
        time: "21:55",
        lat: "26.70",
        long: "-81.50",
        country: "East Timor", // this is a real country i swear (WHY DID COPILOT GENERATE THIS COMMENT??)
        isMilitary: "false",
    }
];

// Function to create cards
function createCard(aircraft) {
    const card = document.createElement("a");
    card.href = "logexp.html";
    card.className = "card";

    // Image, maybe get rid of, doing same image for all for now
    const img = document.createElement("img");
    img.src = "media/737max9.png";
    img.alt = `${aircraft.name} Image`;
    img.className = "card-img";


    const textList = document.createElement("ul");
    textList.className = "card-text";

    const title = document.createElement("h");
    title.textContent = aircraft.model;
    textList.appendChild(title);

    textList.appendChild(document.createElement("br")); // idk why these are here tbh
    textList.appendChild(document.createElement("br"));

    // Add the rest of the info to a list
    const { icao, date, time, lat, long, country, isMilitary } = aircraft;
    const content = { icao, date, time, lat, long, country, isMilitary }
    for (const [key, value] of Object.entries(content)) {
        const listItem = document.createElement("li");
        listItem.textContent = `${key}: ${value}`;
        textList.appendChild(listItem);
    }
    // Add the image to the card
    card.appendChild(img);
    // Add the list to the card
    card.appendChild(textList);

    return card;
}

// Function to render cards in rows of 3
function renderCards(data) {
    const container = document.getElementById("cards-container");
    //container.innerHTML = ""; // Clear any existing cards // IDK what this does??
    let row;
    data.forEach((aircraft, index) => {
        if (index % 3 === 0) {
            // Create a new row every 3 cards
            row = document.createElement("div");
            row.className = "cards-container";
            container.appendChild(row);
        }
        const card = createCard(aircraft);
        row.appendChild(card);
    });
}

renderCards(mockDatabase);


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