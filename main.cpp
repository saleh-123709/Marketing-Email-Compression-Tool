#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <regex>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/stat.h>

using namespace std;

// ============================================================
// RESULT STRUCTURE
// ============================================================

struct CompressionResult {
    string method;
    long long size;
    double saved;
    double ratio;
    double time;
};

// ============================================================
// HTML MINIFICATION
// ============================================================

string minifyHTML(string html)
{
    try {
        // Remove HTML comments
        html = regex_replace(
            html,
            regex("<!--[\\s\\S]*?-->"),
            ""
        );

        // Remove spaces between HTML tags
        html = regex_replace(
            html,
            regex(">\\s+<"),
            "><"
        );

        // Replace multiple spaces with one
        html = regex_replace(
            html,
            regex("\\s+"),
            " "
        );

        // Remove spaces at beginning and end
        size_t start =
            html.find_first_not_of(" \t\n\r");

        if (start == string::npos)
            return "";

        size_t end =
            html.find_last_not_of(" \t\n\r");

        html =
            html.substr(
                start,
                end - start + 1
            );
    }
    catch (...) {
        return html;
    }

    return html;
}

// ============================================================
// FILE SIZE
// ============================================================

long long getFileSize(const string& filename)
{
    struct stat fileStat;

    if (stat(filename.c_str(), &fileStat) != 0)
        return -1;

    return fileStat.st_size;
}

// ============================================================
// WRITE FILE
// ============================================================

bool writeFile(
    const string& filename,
    const string& content
)
{
    ofstream file(
        filename,
        ios::binary
    );

    if (!file)
        return false;

    file.write(
        content.c_str(),
        content.size()
    );

    file.close();

    return true;
}

// ============================================================
// CALCULATE SPACE SAVED
// ============================================================

double calculateSaving(
    long long original,
    long long compressed
)
{
    if (original <= 0)
        return 0;

    return
        ((double)(original - compressed)
        / original) * 100.0;
}

// ============================================================
// CREATE RESULT
// ============================================================

CompressionResult makeResult(
    const string& method,
    long long originalSize,
    long long compressedSize,
    double time
)
{
    CompressionResult result;

    result.method = method;

    result.size = compressedSize;

    result.saved =
        calculateSaving(
            originalSize,
            compressedSize
        );

    result.ratio =
        (double)compressedSize /
        originalSize;

    result.time = time;

    return result;
}

// ============================================================
// HTML MINIFICATION RESULT
// ============================================================

CompressionResult performMinification(
    const string& html,
    long long originalSize
)
{
    auto start =
        chrono::high_resolution_clock::now();

    string minified =
        minifyHTML(html);

    auto end =
        chrono::high_resolution_clock::now();

    chrono::duration<double, milli> duration =
        end - start;

    return makeResult(
        "HTML Minification",
        originalSize,
        minified.size(),
        duration.count()
    );
}

// ============================================================
// RUN SYSTEM COMMAND
// ============================================================

bool runCommand(const string& command)
{
    int result =
        system(command.c_str());

    return result == 0;
}

// ============================================================
// GZIP
// ============================================================

CompressionResult performGZIP(
    const string& inputFile,
    const string& outputFile,
    long long originalSize
)
{
    remove(outputFile.c_str());

    auto start =
        chrono::high_resolution_clock::now();

    string command =
        "gzip -c \"" +
        inputFile +
        "\" > \"" +
        outputFile +
        "\"";

    bool success =
        runCommand(command);

    auto end =
        chrono::high_resolution_clock::now();

    chrono::duration<double, milli> duration =
        end - start;

    if (!success) {
        return makeResult(
            "GZIP",
            originalSize,
            originalSize,
            duration.count()
        );
    }

    long long size =
        getFileSize(outputFile);

    return makeResult(
        "GZIP",
        originalSize,
        size,
        duration.count()
    );
}

// ============================================================
// ZIP
// ============================================================

CompressionResult performZIP(
    const string& inputFile,
    const string& outputFile,
    long long originalSize
)
{
    remove(outputFile.c_str());

    auto start =
        chrono::high_resolution_clock::now();

    string command =
        "zip -q -j \"" +
        outputFile +
        "\" \"" +
        inputFile +
        "\"";

    bool success =
        runCommand(command);

    auto end =
        chrono::high_resolution_clock::now();

    chrono::duration<double, milli> duration =
        end - start;

    if (!success) {
        return makeResult(
            "ZIP",
            originalSize,
            originalSize,
            duration.count()
        );
    }

    long long size =
        getFileSize(outputFile);

    return makeResult(
        "ZIP",
        originalSize,
        size,
        duration.count()
    );
}

// ============================================================
// BROTLI
// ============================================================

CompressionResult performBrotli(
    const string& inputFile,
    const string& outputFile,
    long long originalSize
)
{
    remove(outputFile.c_str());

    auto start =
        chrono::high_resolution_clock::now();

    string command =
        "brotli -q 6 -o \"" +
        outputFile +
        "\" \"" +
        inputFile +
        "\"";

    bool success =
        runCommand(command);

    auto end =
        chrono::high_resolution_clock::now();

    chrono::duration<double, milli> duration =
        end - start;

    if (!success) {
        return makeResult(
            "Brotli",
            originalSize,
            originalSize,
            duration.count()
        );
    }

    long long size =
        getFileSize(outputFile);

    return makeResult(
        "Brotli",
        originalSize,
        size,
        duration.count()
    );
}

// ============================================================
// JSON ESCAPE
// ============================================================

string jsonEscape(const string& text)
{
    string result;

    for (char c : text) {

        if (c == '"')
            result += "\\\"";

        else if (c == '\\')
            result += "\\\\";

        else if (c == '\n')
            result += "\\n";

        else if (c == '\r')
            result += "\\r";

        else if (c == '\t')
            result += "\\t";

        else
            result += c;
    }

    return result;
}

// ============================================================
// CREATE JSON
// ============================================================

string createJSON(
    long long originalSize,
    const vector<CompressionResult>& results
)
{
    string bestMethod = "";
    double bestSaving = -999999;

    for (const auto& result : results) {

        if (result.saved > bestSaving) {

            bestSaving =
                result.saved;

            bestMethod =
                result.method;
        }
    }

    string json = "{";

    json += "\"success\":true,";

    json +=
        "\"originalSize\":" +
        to_string(originalSize) +
        ",";

    json +=
        "\"bestMethod\":\"" +
        jsonEscape(bestMethod) +
        "\",";

    json +=
        "\"bestSaving\":" +
        to_string(bestSaving) +
        ",";

    json += "\"results\":[";

    for (size_t i = 0;
         i < results.size();
         i++) {

        const auto& r =
            results[i];

        json += "{";

        json +=
            "\"method\":\"" +
            jsonEscape(r.method) +
            "\",";

        json +=
            "\"size\":" +
            to_string(r.size) +
            ",";

        json +=
            "\"saved\":" +
            to_string(r.saved) +
            ",";

        json +=
            "\"ratio\":" +
            to_string(r.ratio) +
            ",";

        json +=
            "\"time\":" +
            to_string(r.time);

        json += "}";

        if (i + 1 < results.size())
            json += ",";
    }

    json += "]";

    json += "}";

    return json;
}

// ============================================================
// HTML PAGE
// ============================================================

const string HTML_PAGE = R"HTML(

<!DOCTYPE html>

<html lang="en">

<head>

<meta charset="UTF-8">

<meta name="viewport"
content="width=device-width, initial-scale=1.0">

<title>
Marketing Email Compression Tool
</title>

<style>

* {
    box-sizing: border-box;
}

body {

    margin: 0;

    font-family:
    Arial, sans-serif;

    background:
    #f4f6f8;

    color: #222;
}

.container {

    width: 90%;

    max-width: 1200px;

    margin: 40px auto;
}

.header {

    background: #222;

    color: white;

    padding: 30px;

    border-radius: 15px;

    text-align: center;
}

.header h1 {

    margin: 0;

    font-size: 32px;
}

.header p {

    color: #ddd;
}

.card {

    background: white;

    padding: 25px;

    margin-top: 25px;

    border-radius: 15px;

    box-shadow:
    0 4px 15px
    rgba(0,0,0,0.08);
}

textarea {

    width: 100%;

    height: 300px;

    resize: vertical;

    padding: 15px;

    font-family: monospace;

    font-size: 14px;

    border:
    1px solid #ccc;

    border-radius: 10px;
}

button {

    margin-top: 15px;

    padding: 14px 25px;

    border: none;

    border-radius: 8px;

    background: #222;

    color: white;

    font-size: 16px;

    cursor: pointer;
}

button:hover {

    background: #444;
}

button:disabled {

    background: #999;

    cursor: not-allowed;
}

.loading {

    display: none;

    margin-top: 15px;

    font-weight: bold;
}

.results {

    display: none;
}

table {

    width: 100%;

    border-collapse:
    collapse;

    margin-top: 20px;
}

th, td {

    padding: 14px;

    border-bottom:
    1px solid #ddd;

    text-align: center;
}

th {

    background: #222;

    color: white;
}

.summary {

    display: grid;

    grid-template-columns:
    repeat(3, 1fr);

    gap: 15px;

    margin-top: 20px;
}

.summary-box {

    background: #f1f3f5;

    padding: 20px;

    border-radius: 10px;

    text-align: center;
}

.summary-box h3 {

    margin: 0 0 10px;
}

.summary-box p {

    font-size: 20px;

    font-weight: bold;
}

.error {

    color: #b00020;

    margin-top: 15px;

    font-weight: bold;
}

.footer {

    text-align: center;

    margin-top: 30px;

    color: #777;
}

@media(max-width:700px) {

    .summary {

        grid-template-columns:
        1fr;
    }

    table {

        font-size: 12px;
    }
}

</style>

</head>

<body>

<div class="container">

<div class="header">

<h1>
Marketing Email Compression Tool
</h1>

<p>
Compare HTML Minification, GZIP, ZIP and Brotli
</p>

</div>

<div class="card">

<h2>
Enter Marketing Email HTML
</h2>

<textarea id="emailInput">

<!DOCTYPE html>

<html>

<head>

<title>
Special Marketing Offer
</title>

</head>

<body>

<div style="padding:20px;">

<h1>
Special Offer!
</h1>

<p>
Get 30% discount on our products today.
</p>

<p>
This is a special marketing email.
</p>

<a href="https://example.com">
Shop Now
</a>

</div>

</body>

</html>

</textarea>

<br>

<button
id="compressButton"
onclick="compressEmail()">

COMPRESS EMAIL

</button>

<div
class="loading"
id="loading">

Processing compression methods...

</div>

<div
class="error"
id="error">
</div>

</div>

<div
class="card results"
id="results">

<h2>
Compression Results
</h2>

<div class="summary">

<div class="summary-box">

<h3>
Original Size
</h3>

<p id="originalSize">
-
</p>

</div>

<div class="summary-box">

<h3>
Best Method
</h3>

<p id="bestMethod">
-
</p>

</div>

<div class="summary-box">

<h3>
Maximum Saving
</h3>

<p id="bestSaving">
-
</p>

</div>

</div>

<table>

<thead>

<tr>

<th>
Method
</th>

<th>
Compressed Size
</th>

<th>
Space Saved
</th>

<th>
Compression Ratio
</th>

<th>
Time
</th>

</tr>

</thead>

<tbody id="resultsBody">
</tbody>

</table>

</div>

<div class="card">

<h2>
How It Works
</h2>

<ol>

<li>
User enters marketing email HTML.
</li>

<li>
C++ receives the HTML.
</li>

<li>
HTML is minified.
</li>

<li>
GZIP compression is applied.
</li>

<li>
ZIP compression is applied.
</li>

<li>
Brotli compression is applied.
</li>

<li>
The results are calculated.
</li>

<li>
Results are displayed on the webpage.
</li>

</ol>

</div>

<div class="footer">

Marketing Email Compression Comparison Tool

<br><br>

Developed using C++ and Web Technologies

</div>

</div>

<script>

async function compressEmail() {

    const input =
        document.getElementById(
            "emailInput"
        );

    const button =
        document.getElementById(
            "compressButton"
        );

    const loading =
        document.getElementById(
            "loading"
        );

    const error =
        document.getElementById(
            "error"
        );

    const results =
        document.getElementById(
            "results"
        );

    const body =
        document.getElementById(
            "resultsBody"
        );

    const html =
        input.value;

    error.innerHTML = "";

    if (html.trim().length === 0) {

        error.innerHTML =
            "Please enter HTML email content.";

        return;
    }

    button.disabled = true;

    loading.style.display =
        "block";

    results.style.display =
        "none";

    try {

        const response =
            await fetch(
                "/compress",
                {
                    method: "POST",

                    headers: {
                        "Content-Type":
                        "text/plain"
                    },

                    body: html
                }
            );

        const data =
            await response.json();

        if (!data.success) {

            throw new Error(
                data.error
            );
        }

        document.getElementById(
            "originalSize"
        ).innerText =
            formatBytes(
                data.originalSize
            );

        document.getElementById(
            "bestMethod"
        ).innerText =
            data.bestMethod;

        document.getElementById(
            "bestSaving"
        ).innerText =
            data.bestSaving.toFixed(2)
            + "%";

        body.innerHTML = "";

        data.results.forEach(
            function(item) {

                const row =
                    document.createElement(
                        "tr"
                    );

                row.innerHTML = `

                    <td>
                    <strong>
                    ${item.method}
                    </strong>
                    </td>

                    <td>
                    ${formatBytes(item.size)}
                    </td>

                    <td>
                    ${item.saved.toFixed(2)}%
                    </td>

                    <td>
                    ${item.ratio.toFixed(4)}
                    </td>

                    <td>
                    ${item.time.toFixed(3)} ms
                    </td>

                `;

                body.appendChild(row);
            }
        );

        results.style.display =
            "block";

    }

    catch (err) {

        error.innerText =
            "Error: " +
            err.message;
    }

    button.disabled =
        false;

    loading.style.display =
        "none";
}


function formatBytes(bytes) {

    if (bytes < 1024)
        return bytes + " B";

    if (bytes < 1024 * 1024)
        return (
            bytes / 1024
        ).toFixed(2) + " KB";

    return (
        bytes /
        (1024 * 1024)
    ).toFixed(2) + " MB";
}

</script>

</body>

</html>

)HTML";

// ============================================================
// HTTP RESPONSE
// ============================================================

string httpResponse(
    const string& content,
    const string& contentType
)
{
    string response;

    response +=
        "HTTP/1.1 200 OK\r\n";

    response +=
        "Content-Type: " +
        contentType +
        "\r\n";

    response +=
        "Content-Length: " +
        to_string(content.size()) +
        "\r\n";

    response +=
        "Access-Control-Allow-Origin: *\r\n";

    response +=
        "Connection: close\r\n";

    response += "\r\n";

    response += content;

    return response;
}

// ============================================================
// ERROR RESPONSE
// ============================================================

string errorResponse(
    const string& message
)
{
    string json =
        "{"
        "\"success\":false,"
        "\"error\":\"" +
        jsonEscape(message) +
        "\""
        "}";

    return httpResponse(
        json,
        "application/json"
    );
}

// ============================================================
// GET CONTENT LENGTH
// ============================================================

long long getContentLength(
    const string& request
)
{
    string key =
        "Content-Length:";

    size_t pos =
        request.find(key);

    if (pos == string::npos)
        return 0;

    pos += key.length();

    while (
        pos < request.size() &&
        (
            request[pos] == ' ' ||
            request[pos] == '\t'
        )
    )
        pos++;

    size_t end =
        request.find(
            "\r\n",
            pos
        );

    if (end == string::npos)
        return 0;

    string value =
        request.substr(
            pos,
            end - pos
        );

    return stoll(value);
}

// ============================================================
// MAIN SERVER
// ============================================================

int main()
{
    cout << endl;

    cout <<
        "============================================"
        << endl;

    cout <<
        " MARKETING EMAIL COMPRESSION TOOL"
        << endl;

    cout <<
        "============================================"
        << endl;

    // Get port from environment
    int port = 8080;

    const char* envPort =
        getenv("PORT");

    if (envPort != nullptr)
        port = atoi(envPort);

    // Create socket
    int serverSocket =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

    if (serverSocket < 0) {

        cerr <<
            "Error creating socket."
            << endl;

        return 1;
    }

    int option = 1;

    setsockopt(
        serverSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &option,
        sizeof(option)
    );

    // Server address
    sockaddr_in serverAddress{};

    serverAddress.sin_family =
        AF_INET;

    serverAddress.sin_addr.s_addr =
        INADDR_ANY;

    serverAddress.sin_port =
        htons(port);

    // Bind
    if (
        bind(
            serverSocket,
            (struct sockaddr*)&serverAddress,
            sizeof(serverAddress)
        ) < 0
    )
    {
        cerr <<
            "Error binding server."
            << endl;

        close(serverSocket);

        return 1;
    }

    // Listen
    if (
        listen(
            serverSocket,
            10
        ) < 0
    )
    {
        cerr <<
            "Error starting server."
            << endl;

        close(serverSocket);

        return 1;
    }

    cout << endl;

    cout <<
        "Server running on port "
        << port
        << endl;

    cout <<
        "Waiting for browser connection..."
        << endl;

    // Server loop
    while (true)
    {
        sockaddr_in clientAddress{};

        socklen_t clientLength =
            sizeof(clientAddress);

        int clientSocket =
            accept(
                serverSocket,
                (struct sockaddr*)&clientAddress,
                &clientLength
            );

        if (clientSocket < 0)
            continue;

        string request;

        char buffer[8192];

        int bytesReceived;

        while (
            (
                bytesReceived =
                recv(
                    clientSocket,
                    buffer,
                    sizeof(buffer),
                    0
                )
            ) > 0
        )
        {
            request.append(
                buffer,
                bytesReceived
            );

            size_t headerEnd =
                request.find(
                    "\r\n\r\n"
                );

            if (
                headerEnd !=
                string::npos
            )
            {
                long long contentLength =
                    getContentLength(
                        request
                    );

                size_t bodyStart =
                    headerEnd + 4;

                if (
                    request.size() -
                    bodyStart >=
                    (size_t)contentLength
                )
                    break;
            }

            if (
                request.size() >
                20 * 1024 * 1024
            )
                break;
        }

        // ====================================================
        // GET /
        // ====================================================

        if (
            request.rfind(
                "GET / ",
                0
            ) == 0
        )
        {
            string response =
                httpResponse(
                    HTML_PAGE,
                    "text/html; charset=UTF-8"
                );

            send(
                clientSocket,
                response.c_str(),
                response.size(),
                0
            );

            close(clientSocket);

            continue;
        }

        // ====================================================
        // POST /compress
        // ====================================================

        if (
            request.rfind(
                "POST /compress",
                0
            ) == 0
        )
        {
            size_t headerEnd =
                request.find(
                    "\r\n\r\n"
                );

            if (
                headerEnd ==
                string::npos
            )
            {
                string response =
                    errorResponse(
                        "Invalid HTTP request."
                    );

                send(
                    clientSocket,
                    response.c_str(),
                    response.size(),
                    0
                );

                close(clientSocket);

                continue;
            }

            long long contentLength =
                getContentLength(
                    request
                );

            size_t bodyStart =
                headerEnd + 4;

            if (
                request.size() -
                bodyStart <
                (size_t)contentLength
            )
            {
                string response =
                    errorResponse(
                        "Incomplete request."
                    );

                send(
                    clientSocket,
                    response.c_str(),
                    response.size(),
                    0
                );

                close(clientSocket);

                continue;
            }

            string html =
                request.substr(
                    bodyStart,
                    contentLength
                );

            if (html.empty())
            {
                string response =
                    errorResponse(
                        "HTML input is empty."
                    );

                send(
                    clientSocket,
                    response.c_str(),
                    response.size(),
                    0
                );

                close(clientSocket);

                continue;
            }

            // Temporary files
            string base =
                "/tmp/email_compression_" +
                to_string(getpid());

            string inputFile =
                base + ".html";

            string gzipFile =
                base + ".gz";

            string zipFile =
                base + ".zip";

            string brotliFile =
                base + ".br";

            // Write original HTML
            if (
                !writeFile(
                    inputFile,
                    html
                )
            )
            {
                string response =
                    errorResponse(
                        "Cannot create temporary file."
                    );

                send(
                    clientSocket,
                    response.c_str(),
                    response.size(),
                    0
                );

                close(clientSocket);

                continue;
            }

            long long originalSize =
                html.size();

            vector<CompressionResult>
                results;

            // Minification
            results.push_back(
                performMinification(
                    html,
                    originalSize
                )
            );

            // GZIP
            results.push_back(
                performGZIP(
                    inputFile,
                    gzipFile,
                    originalSize
                )
            );

            // ZIP
            results.push_back(
                performZIP(
                    inputFile,
                    zipFile,
                    originalSize
                )
            );

            // Brotli
            results.push_back(
                performBrotli(
                    inputFile,
                    brotliFile,
                    originalSize
                )
            );

            // JSON
            string json =
                createJSON(
                    originalSize,
                    results
                );

            string response =
                httpResponse(
                    json,
                    "application/json"
                );

            send(
                clientSocket,
                response.c_str(),
                response.size(),
                0
            );

            // Remove temporary files
            remove(
                inputFile.c_str()
            );

            remove(
                gzipFile.c_str()
            );

            remove(
                zipFile.c_str()
            );

            remove(
                brotliFile.c_str()
            );

            close(
                clientSocket
            );

            continue;
        }

        // Unknown request
        string response =
            errorResponse(
                "Page not found."
            );

        send(
            clientSocket,
            response.c_str(),
            response.size(),
            0
        );

        close(
            clientSocket
        );
    }

    close(
        serverSocket
    );

    return 0;
}
