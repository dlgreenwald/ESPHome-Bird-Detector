static const char uploadScript[] PROGMEM = R"==x==(
<table class="fixed" border="0">
    <col width="1000px" /><col width="500px" />
    <tr><td>
        <h2>ESP32 File Server</h2>
    </td><td>
        <table border="0">
            <tr>
                <td>
                    <label for="newfile">Upload a file</label>
                </td>
                <td colspan="2">
                    <input id="newfile" type="file" onchange="setpath()" style="width:100%;">
                </td>
            </tr>
            <tr>
                <td>
                    <label for="filepath">Set path on server</label>
                </td>
                <td>
                    <input id="filepath" type="text" style="width:100%;">
                </td>
                <td>
                    <button id="upload" type="button" onclick="upload()">Upload</button>
                </td>
            </tr>
        </table>
    </td></tr>
</table>
<script>
function setpath() {
    var default_path = document.getElementById("newfile").files[0].name;
    document.getElementById("filepath").value = default_path;
}
function upload() {
    var filePath = document.getElementById("filepath").value;
    var upload_path = "/upload/" + filePath;
    var fileInput = document.getElementById("newfile").files;

    /* Max size of an individual file. Make sure this
     * value is same as that set in file_server.c */
    var MAX_FILE_SIZE = 200*1024;
    var MAX_FILE_SIZE_STR = "200KB";

    if (fileInput.length == 0) {
        alert("No file selected!");
    } else if (filePath.length == 0) {
        alert("File path on server is not set!");
    } else if (filePath.indexOf(' ') >= 0) {
        alert("File path on server cannot have spaces!");
    } else if (filePath[filePath.length-1] == '/') {
        alert("File name not specified after path!");
    } else if (fileInput[0].size > 200*1024) {
        alert("File size must be less than 200KB!");
    } else {
        document.getElementById("newfile").disabled = true;
        document.getElementById("filepath").disabled = true;
        document.getElementById("upload").disabled = true;

        var file = fileInput[0];
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (xhttp.readyState == 4) {
                if (xhttp.status == 200) {
                    document.open();
                    document.write(xhttp.responseText);
                    document.close();
                } else if (xhttp.status == 0) {
                    alert("Server closed the connection abruptly!");
                    location.reload()
                } else {
                    alert(xhttp.status + " Error!\n" + xhttp.responseText);
                    location.reload()
                }
            }
        };
        xhttp.open("POST", upload_path, true);
        xhttp.send(file);
    }
}
</script>
    )==x==";