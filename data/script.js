// Get current sensor readings when the page loads  
window.addEventListener('load', getReadings);

// Function to add date and time of last update.

function updateDateTime() {

    var currentdate = new Date();
    var datetime = currentdate.getDate() + "/"
        + (currentdate.getMonth() + 1) + "/"
        + currentdate.getFullYear() + " at "
        + currentdate.getHours() + ":"
        + currentdate.getMinutes() + ":"
        + currentdate.getSeconds();

    document.getElementById("update-time").innerHTML = datetime;
    console.log(datetime);

} // Close function.

// Function to get current readings on the webpage when it loads for the first time.
function getReadings() {

    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {

            var myObj = JSON.parse(this.responseText);
            console.log(myObj);
            document.getElementById("sessionTimeArray1").innerHTML = myObj.sessionTimeArray1;
            document.getElementById("sessionTimeArray2").innerHTML = myObj.sessionTimeArray2;
            document.getElementById("sessionTimeArray3").innerHTML = myObj.sessionTimeArray3;
            document.getElementById("sessionTimeArray4").innerHTML = myObj.sessionTimeArray4;
            document.getElementById("sessionTimeArray5").innerHTML = myObj.sessionTimeArray5;
            document.getElementById("sessionTimeArray6").innerHTML = myObj.sessionTimeArray6;
            document.getElementById("sessionTimeArray7").innerHTML = myObj.sessionTimeArray7;
            document.getElementById("distanceTravelledArray1").innerHTML = myObj.distanceTravelledArray1;
            document.getElementById("distanceTravelledArray2").innerHTML = myObj.distanceTravelledArray2;
            document.getElementById("distanceTravelledArray3").innerHTML = myObj.distanceTravelledArray3;
            document.getElementById("distanceTravelledArray4").innerHTML = myObj.distanceTravelledArray4;
            document.getElementById("distanceTravelledArray5").innerHTML = myObj.distanceTravelledArray5;
            document.getElementById("distanceTravelledArray6").innerHTML = myObj.distanceTravelledArray6;
            document.getElementById("distanceTravelledArray7").innerHTML = myObj.distanceTravelledArray7;
            updateDateTime();
        }
    };

    xhr.open("GET", "/readings", true);
    xhr.send();

} // Close function.

// Create an Event Source to listen for events
if (!!window.EventSource) {
    var source = new EventSource('/events');

    source.addEventListener('open', function (e) {
        console.log("Events Connected");
    }, false);

    source.addEventListener('error', function (e) {
        if (e.target.readyState != EventSource.OPEN) {
            console.log("Events Disconnected");
        }
    }, false);

    source.addEventListener('new_readings', function (e) {
        console.log("new_readings", e.data);
        var obj = JSON.parse(e.data);
        document.getElementById("sessionTimeArray1").innerHTML = Obj.sessionTimeArray1;
        document.getElementById("sessionTimeArray2").innerHTML = Obj.sessionTimeArray2;
        document.getElementById("sessionTimeArray3").innerHTML = Obj.sessionTimeArray3;
        document.getElementById("sessionTimeArray4").innerHTML = Obj.sessionTimeArray4;
        document.getElementById("sessionTimeArray5").innerHTML = Obj.sessionTimeArray5;
        document.getElementById("sessionTimeArray6").innerHTML = Obj.sessionTimeArray6;
        document.getElementById("sessionTimeArray7").innerHTML = Obj.sessionTimeArray7;
        document.getElementById("distanceTravelledArray1").innerHTML = Obj.distanceTravelledArray1;
        document.getElementById("distanceTravelledArray2").innerHTML = Obj.distanceTravelledArray2;
        document.getElementById("distanceTravelledArray3").innerHTML = Obj.distanceTravelledArray3;
        document.getElementById("distanceTravelledArray4").innerHTML = Obj.distanceTravelledArray4;
        document.getElementById("distanceTravelledArray5").innerHTML = Obj.distanceTravelledArray5;
        document.getElementById("distanceTravelledArray6").innerHTML = Obj.distanceTravelledArray6;
        document.getElementById("distanceTravelledArray7").innerHTML = Obj.distanceTravelledArray7;
        updateDateTime();
    }, false);
}