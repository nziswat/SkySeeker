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
function aircraftClick(e, aircraft) {
    selectedAircraft = aircraft; //set globally selected aircraft to what we just clicked
    updateDetailTable(aircraft);
    aircraft.checkICAOData();
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


}
function updateDetailTable(aircraft) {
    const container = document.getElementById('detailTable');
    const table = container.querySelector('table'); // strange things...
    table.rows[1].cells[1].textContent = aircraft.ID;
    table.rows[2].cells[1].textContent = aircraft.lat.toFixed(3);
    table.rows[3].cells[1].textContent = aircraft.long.toFixed(3);
    table.rows[4].cells[1].textContent = aircraft.head.toFixed(2);
    table.rows[5].cells[1].textContent = aircraft.alt;
    table.rows[6].cells[1].textContent = aircraft.speed.toFixed(0);

}

function startStopDriver() {
    window.cefQuery({
        request: "startStopDriver",
        onSuccess: function (response) {
            console.log("Driver toggled: " + response);
        },
        onFailure: function (error_code, error_message) {
            console.error("CEF Query Failed:", error_code, error_message);
        }
    });
}


// Function to update Aircraft/flight info
function receiveSignal(map, ID, lat, long, head, alt, speed, fflag) {

    // If it does not exist, create a new aircraft object
    if (hashMap.get(ID) == undefined) {

        // Create new aircraft object
        let newAircraft = new Aircraft(ID);

        // Insert it into the hash map
        hashMap.set(ID, newAircraft);

        // Check if lat and long are defined
        if ((lat != undefined) && (long != undefined)) {
            newAircraft.updateLatLong(lat, long, fflag);
        }
        //console.log("Updated Aircraft Data3:", "icao:", newAircraft.ID, "lat:", newAircraft.lat, "long:", newAircraft.long,
        //    "head:", newAircraft.head, "alt:", newAircraft.alt, "speed:", newAircraft.speed);

    }
    // If it does exist, update the aircraft object
    else {
        let existingAircraft = hashMap.get(ID);
        let properties = { head, alt, speed };
        for (let key in properties) {
            if (properties[key] != undefined) {
                existingAircraft[key] = properties[key];  // Correct property update
            }
        }

        // Check if lat and long are defined
        if ((lat != undefined) && (long != undefined)) {
            // Try to update them
            existingAircraft.updateLatLong(lat, long, fflag);
        }


        // Update the movingMarker and polyline
        if ((existingAircraft.lat != undefined) && (existingAircraft.long != undefined)) {

            // If moving marker and polyline are undefined, initialize them
            if (existingAircraft.movingMarker == undefined) {
                // Initialize moving marker if heading is known
                if (existingAircraft.head != undefined) {
                    existingAircraft.movingMarker = L.Marker.movingMarker(
                        [[existingAircraft.lat, existingAircraft.long]], [0], { icon: planeIcon }
                    ).addTo(map);

                    existingAircraft.movingMarker.on('click', function (e) { //binds clicking the marker to the click function
                        aircraftClick(e, existingAircraft);
                    });
                }
                // Create a polyline to track the plane's path
                existingAircraft.polyline = L.polyline([[existingAircraft.lat, existingAircraft.long]], { color: 'blue', opacity: 0 }).addTo(map);
            }
            // Otherwise, just update the movingMarker and polyline like normal
            else {
                existingAircraft.movingMarker.setLatLng([existingAircraft.lat, existingAircraft.long]);
                existingAircraft.polyline.addLatLng([existingAircraft.lat, existingAircraft.long]);
            }
        }
        if ((head != undefined) && (existingAircraft.movingMarker != undefined)) { //???: couldn't this be moved?
            existingAircraft.movingMarker.setRotationAngle(existingAircraft.head + 135); // +135 rotate for given icon
        }

        if (selectedAircraft != undefined) { //if there is a selected aircraft-
            if (existingAircraft.ID == selectedAircraft.ID) {//-that is the same as the new information we just got-
                updateDetailTable(existingAircraft);//be sure to update the table
            }
        }
        //console.log("Updated Aircraft Data3:", "icao:", existingAircraft.ID, "lat:", existingAircraft.lat, "long:", existingAircraft.long,
        //   "head:", existingAircraft.head, "alt:", existingAircraft.alt, "speed:", existingAircraft.speed);
    }

}

function updateAircraftData(data) {
    let aircraftData = data;
    let icao = aircraftData.icao;
    let speed;
    //console.log("Updated Aircraft Data1:", "icao:", icao, "lat:", aircraftData.raw_latitude, "long:", aircraftData.raw_longitude, "head:", aircraftData.heading, "alt:", aircraftData.altitude);
    if ((aircraftData.heading == null) || (aircraftData.heading == undefined)) {
        if ((aircraftData.ew_velocity != null) || (aircraftData.ew_velocity != undefined)) {
            //find heading, first get parity
            let nsVelocity = aircraftData.ns_velocity * (2 * aircraftData.ns_dir - 1);  //parity is given as 0, 1 not -1, 1
            let ewVelocity = aircraftData.ew_velocity * (2 * aircraftData.ew_dir - 1);  //this just makes it -1 or 1
            aircraftData.heading = Math.atan2(ewVelocity, nsVelocity) * 180 / Math.PI;
            if (aircraftData.heading < 0) {
                aircraftData.heading += 360 // turn negatives into positives
            }
        }
    }

    if ((aircraftData.ew_velocity != null) || (aircraftData.ew_velocity != undefined)) {
        // Calculate speed
        speed = Math.sqrt(aircraftData.ns_velocity ** 2 + aircraftData.ew_velocity ** 2);
    }

    // Set null values to undefined
    // Note: unsure if they are null or undefined by default
    if (aircraftData.raw_latitude == null) {
        aircraftData.raw_latitude = undefined;
        aircraftData.raw_longitude = undefined;
    }
    if (aircraftData.heading == null) {
        aircraftData.heading = undefined;
    }
    if (aircraftData.altitude == null) {
        aircraftData.altitude = undefined;
    }
    if (speed == null) {
        speed = undefined;
    }
    if (aircraftData.fflag == null) {
        aircraftData.fflag = undefined;
    }
    //console.log("Updated Aircraft Data2:", "icao:", icao, "lat:", aircraftData.raw_latitude, "long:", aircraftData.raw_longitude, "ns vel:", aircraftData.ns_velocity, "ew vel:",
    //    aircraftData.ew_velocity, "head:", aircraftData.heading, "alt:", aircraftData.altitude, "speed:", speed);
    receiveSignal(map, aircraftData.icao, aircraftData.raw_latitude, aircraftData.raw_longitude, aircraftData.heading, aircraftData.altitude, speed, aircraftData.fflag);
    forceUpdateAircraftTable(hashMap)
    //printHashMap(hashMap);
}

//debug functions vvvv
function printHashMap(hmap) {
    hmap.forEach((value, key) => {
        console.log(`HMAP Debug: ${key}`);
        console.log(`HMAP Debug info:`, value);
    });
}
function debugcheckICAOData(stwing) {
    console.log(`Trying to check ICAO data for ${stwing}`);
    let query_string = `getICAOData${stwing}`;
    console.log(`trynna check${query_string}`)
    window.cefQuery({
        request: query_string,
        onSuccess: function (response) {
            console.log("ICAO DATA GET!" + response);
        },
        onFailure: function (error_code, error_message) {
            console.error("Literally could not get the frickin data wtf kevin", error_code, error_message);
        }
    });


}

window.debugCall = function (input) {
    //console.log("Debug input received, input is as follows:");
    console.log(input);
};