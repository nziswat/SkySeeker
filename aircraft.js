// Aircraft Class
// Properties: ID, Latitude, Longitude, Heading, Altitude, Speed

class Aircraft { 

    // Constructor function to build Aircraft object
    constructor(ID, lat, long, head, alt, speed) {
        this._ID = ID;
        this._lat = lat;
        this._long = long;
        this._head = head;
        this._alt = alt;
        this._speed = speed;
    }

    // Getters and Setters

    // ID
    set ID(ID) { this._ID = ID; }
    get ID() { return this._ID; }

    // Latitude
    set lat(lat) { this._lat = lat; }
    get lat() { return this._lat; }

    // Longitude
    set long(long) { this._long = long; }
    get long() { return this._long; }

    // Heading
    set head(head) { this._head = head; }
    get head() { return this._head; }

    // Altitude
    set alt(alt) { this._alt = alt; }
    get alt() { return this._alt; }

    // Speed
    set speed(speed) { this._speed = speed; }
    get speed() { return this._speed; }

}
