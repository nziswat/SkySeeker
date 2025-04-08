// Main script for SkySeeker

// Function to initialize the map
function initializeMap(containerId, center, zoom) {

    var options = {};
    options.worldCopyJump = true; // Makes the markers repeat

    // Initialize the map
    const map = L.map(containerId, options).setView(center, zoom);

    // Add a tile layer
    const tiles = L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
        maxZoom: 19, // Limits how far user can zoom in
        minZoom: 3,  // Limits how far user can zoom out
        attribution: '&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>'
    }).addTo(map);

    return map;
}

// initialize the table of all aircraft
function createAircraftTable() {
    const container = document.getElementById('aircraftTable');

    // Create table and headers
    const table = document.createElement('table');
    const headerRow = table.insertRow();
    ['ICAO', 'Heading', 'Altitude', 'Speed', 'Lat', 'Long'].forEach(text => {
        const th = document.createElement('th');
        th.textContent = text;
        headerRow.appendChild(th);
    });
    container.appendChild(table);
}


// TODO: Probably make and use the function below if needed for better computation speed. Also there's probably a better way to do the other functions too, but rn it seems like it runs fine
//update the aircraft table with a single aircraft object
function updateAircraftTable(data) {


}

//repopulate the entire table with all data from the hashmap
function forceUpdateAircraftTable(map) {
    const container = document.getElementById('aircraftTable');
    const table = container.firstChild; //the actual table should be the only child

    // check existing ICAOs
    const existingIcaos = new Set();
    for (let i = 0; i < table.rows.length; i++) {
        const icao = table.rows[i].cells[0].textContent;
        existingIcaos.add(icao);
    }
    map.forEach((value, key) => { //put in the ICAOs from the hashmap
        let row;
        if (!existingIcaos.has(key)) {
            row = table.insertRow();
            row.insertCell().textContent = key;
            row.insertCell() // this looks stupid but it's just initializing the cells for each aircraft so you can actually assign stuff to it 
            row.insertCell()
            row.insertCell()
            row.insertCell()
            row.insertCell()
        } else {
            for (const frow of table.rows) {
                if (frow.cells[0].textContent.includes(key)) {
                    row = frow;
                }
            }
        }
        row.cells[1].textContent = value.head !== undefined ? value.head.toFixed(0) : 'N/A'; 
        row.cells[2].textContent = value.alt !== undefined ? value.alt : 'N/A';
        row.cells[3].textContent = value.speed !== undefined ? value.speed.toFixed(0) : 'N/A';
        row.cells[4].textContent = value.lat !== undefined ? value.lat.toFixed(2) : 'N/A';
        row.cells[5].textContent = value.long !== undefined ? value.long.toFixed(2) : 'N/A';
        //console.log(value)


    }
    );
    sortTable() //call the sorting function
    }

    //sort the table by alphabetical ICAO order
function sortTable() {
    const container = document.getElementById('aircraftTable');
    const table = container.firstChild;
    const rows = Array.from(table.rows).slice(1); //skip index 0 / the header

    rows.sort((a, b) => {
        const icaoA = a.cells[0].textContent.toUpperCase();
        const icaoB = b.cells[0].textContent.toUpperCase();
        return icaoA.localeCompare(icaoB);
    });

    // remove all but the header
    while (table.rows.length > 1) {
        table.deleteRow(1);
    }
    //re insert now sorted rows
    rows.forEach(row => {
        table.appendChild(row);
        Array.from(row.cells).forEach(cell => {
            if (cell.textContent === 'N/A') {
                cell.style.backgroundColor = 'darkgrey'; // highlight N/A cells with dark grey
            } else {
                cell.style.backgroundColor = ''; //reset if it's not N/A
            }
        });
    });
    }
    

//use when clicking on a single plane
function updateDetailTable(e, aircraft) {
    const container = document.getElementById('detailTable');
    const table = container.firstChild; //the actual table should be the only child
    selectedAircraft = aircraft;

    // Make all polylines transparent/invisible
    hashMap.forEach((value, key) => {
        if (value.polyline) { 
            value.polyline.setStyle({
                opacity: 0
            });
        }
    });
    // Make selected aircraft polyline visible
    aircraft.polyline.setStyle({
        opacity: 1
    });

    document.getElementById('planeID').textContent = aircraft.ID;
    document.getElementById('planeLat').textContent = aircraft.lat.toFixed(3);
    document.getElementById('planeLong').textContent = aircraft.long.toFixed(3);
    document.getElementById('planeHead').textContent = aircraft.head.toFixed(2);
    document.getElementById('planeAlt').textContent = aircraft.alt;
    document.getElementById('planeSpeed').textContent = aircraft.speed.toFixed(0);

}

//debug
function printHashMap(hmap) {
    hmap.forEach((value, key) => {
        console.log(`HMAP Debug: ${key}`);
        console.log(`HMAP Debug info:`, value);
    });
}

let selectedAircraft;