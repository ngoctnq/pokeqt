const ranks = ["2", "3", "4", "5", "6", "7", "8", "9", "T", "J", "Q", "K", "A"];
const rankNames = ["2", "3", "4", "5", "6", "7", "8", "9", "Ten", "Jack", "Queen", "King", "Ace"];
const gradientColors = ["#0e0e0e", "#3B0F6F", "#8C2981", "#DE4968", "#FE9F6D", "#FCFDBF"];

var cardName = '';
var winProb = '';
var tieProb = '';
var handValue = '';
var currCell = null;
var equity = '';
var lossProb = '';

var numPlayers = null;
var eventSource = null;

function toggle_help() {
    const helpDiv = document.getElementById("help");
    const isHidden = helpDiv.getAttribute("is_hidden") === "true";
    helpDiv.setAttribute("is_hidden", !isHidden);
}

// sometimes the user clicks outside the table, we want to remove all selected cells
// (and it bugs out sometimes)
function removeAllSelected() {
    const selectedCells = document.querySelectorAll(".selected");
    selectedCells.forEach((cell) => {
        cell.classList.remove("selected");
    });
    currCell = null;
    cardName = '';
    winProb = '';
    tieProb = '';
    lossProb = '';
    equity = '';
}

function resetLegend() {
    // clear the right panel
    document.getElementById("hand-name").innerText = cardName || "Select a hand";
    document.getElementById("wins").innerText = winProb || "-";
    document.getElementById("ties").innerText = tieProb || "-";
    document.getElementById("losses").innerText = lossProb || "-";
    document.getElementById("equity").innerText = equity || "-";
}

function toggle(cell) {
    if (currCell === cell) {
        removeAllSelected();
    } else {
        removeAllSelected();
        cardName = document.getElementById("hand-name").innerText;
        winProb = document.getElementById("wins").innerText;
        tieProb = document.getElementById("ties").innerText;
        lossProb = document.getElementById("losses").innerText;
        equity = document.getElementById("equity").innerText;

        currCell = cell;
        currCell.classList.add("selected");
    }
}

function getColorForValue(value) {
    if (value <= 0) {
        return gradientColors[0];
    }
    if (value >= 1) {
        return gradientColors[gradientColors.length - 1];
    }
    const breakpoints = [0];
    for (let i = 1; i < gradientColors.length - 1; i++) {
        breakpoints.push(i / (gradientColors.length - 1));
    }
    breakpoints.push(1);

    for (let i = 0; i < breakpoints.length - 1; i++) {
        if (value >= breakpoints[i] && value <= breakpoints[i + 1]) {
            const ratio = (value - breakpoints[i]) / (breakpoints[i + 1] - breakpoints[i]);
            // Interpolate color
            const color1 = hexToRgb(gradientColors[i]);
            const color2 = hexToRgb(gradientColors[i + 1]);
            const r = Math.round(color1.r + ratio * (color2.r - color1.r));
            const g = Math.round(color1.g + ratio * (color2.g - color1.g));
            const b = Math.round(color1.b + ratio * (color2.b - color1.b));
            return `rgb(${r},${g},${b})`;
        }
    }
}

function hexToRgb(hex) {
    const bigint = parseInt(hex.slice(1), 16);
    const r = (bigint >> 16) & 255;
    const g = (bigint >> 8) & 255;
    const b = bigint & 255;
    return { r, g, b };
}

function submit(element) {
    numPlayers = document.getElementById("num-players").value;
    if (numPlayers < 2 || numPlayers > 9) return;
    startListening(numPlayers);
}

function populate(cell) {
    const cardDiv = document.getElementById("hand-name");
    const winDiv = document.getElementById("wins");
    const tieDiv = document.getElementById("ties");
    const lossDiv = document.getElementById("losses");
    const equityDiv = document.getElementById("equity");

    if (cell.id[0] != cell.id[1]) {
        cardDiv.innerText = rankNames[ranks.indexOf(cell.id[0])] + "-" + rankNames[ranks.indexOf(cell.id[1])] + (cell.id.length === 3 ? (cell.id[2] == 's' ? " Suited" : " Offsuit") : "");
    } else {
        cardDiv.innerText = "Pair of " + rankNames[ranks.indexOf(cell.id[0])] + "s";
    }
    equityDiv.innerText = cell.querySelector(".hand-value").textContent;
    if (equityDiv.innerText === "-") {
        winDiv.innerText = "-";
        tieDiv.innerText = "-";
        lossDiv.innerText = "-";
    } else {
        const overlayDiv = cell.querySelector(".draw-overlay");
        const leftPercent = parseFloat(overlayDiv.style.left);
        const widthPercent = parseFloat(overlayDiv.style.width);
        // measurement already in percentage
        winDiv.innerText = leftPercent.toFixed(3) + "%";
        tieDiv.innerText = widthPercent.toFixed(3) + "%";
        lossDiv.innerText = (100 - (leftPercent + widthPercent)).toFixed(3) + "%";
    }
}

/* THIS IS THE ORIGINAL CODE FOR LIVE PREVIEW, NOT USED ANYMORE
function startListening(numPlayers) {
    if (eventSource) {
        eventSource.close();
    }

    eventSource = new EventSource("/sse_" + numPlayers);

    eventSource.onopen = function () {
        console.log("Listening for SSE...");
    };

    eventSource.onmessage = function (event) {
        const data = JSON.parse(event.data);
        // loop through key-value pairs in data
        let minval = Infinity;
        let maxval = -Infinity;
        for (const key in data) {
            if (data.hasOwnProperty(key)) {
                const count = data[key]["total"];
                if (count > 0) {
                    const value = data[key]["equity_sum"] / count;
                    if (value < minval) minval = value;
                    if (value > maxval) maxval = value;
                    document.getElementById(key).querySelector(".hand-value").textContent = value.toFixed(3);
                }
                else {
                    document.getElementById(key).querySelector(".hand-value").textContent = "-";
                }
            }
        }
        for (const key in data) {
            if (data.hasOwnProperty(key)) {
                const count = data[key]["total"] || 1;
                const overlay = document.getElementById(key).querySelector(".draw-overlay");
                const value = data[key]["equity_sum"] / count;
                const normalizedValue = (value - minval) / (maxval - minval);
                const color = getColorForValue(normalizedValue);

                const winProb = data[key]["win_count"] / count;
                const tieProb = data[key]["tie_count"] / count;
                overlay.style.left = winProb * 100 + "%";
                overlay.style.width = tieProb * 100 + "%";

                document.getElementById(key).style.backgroundColor = color;
                if (normalizedValue < 0.7) {
                    overlay.style.backgroundColor = "#fff";
                    document.getElementById(key).querySelector(".hand-name").style.color = "#eaeaea";
                    document.getElementById(key).querySelector(".hand-value").style.color = "#eaeaea";
                } else {
                    overlay.style.backgroundColor = "#000";
                    document.getElementById(key).querySelector(".hand-name").style.color = "#1a1a1a";
                    document.getElementById(key).querySelector(".hand-value").style.color = "#1a1a1a";
                }
            }
        }
        document.getElementById("minval").textContent = minval.toFixed(3);
        document.getElementById("maxval").textContent = maxval.toFixed(3);
    };

    eventSource.onerror = function (err) {
        console.error("SSE error", err);
        eventSource.close();
        startListening(numPlayers);
    };
}
*/

function startListening(numPlayers) {
    // load a json file on the internet
    fetch(`results/${numPlayers}p.json`)
        .then(response => response.json())
        .then(data => {
            // loop through key-value pairs in data
            let minval = Infinity;
            let maxval = -Infinity;
            for (const key in data) {
                if (data.hasOwnProperty(key)) {
                    const value = data[key]["equity"];
                    if (value < minval) minval = value;
                    if (value > maxval) maxval = value;
                    document.getElementById(key).querySelector(".hand-value").textContent = value.toFixed(3);
                }
                else {
                    document.getElementById(key).querySelector(".hand-value").textContent = "-";
                }
            }
            for (const key in data) {
                if (data.hasOwnProperty(key)) {
                    const overlay = document.getElementById(key).querySelector(".draw-overlay");
                    const value = data[key]["equity"];
                    const normalizedValue = (value - minval) / (maxval - minval);
                    const color = getColorForValue(normalizedValue);

                    const winProb = data[key]["win"];
                    const lossProb = data[key]["loss"];
                    const tieProb = 100 - winProb - lossProb;
                    overlay.style.left = winProb + "%";
                    overlay.style.width = tieProb + "%";

                    document.getElementById(key).style.backgroundColor = color;
                    if (normalizedValue < 0.7) {
                        overlay.style.backgroundColor = "#fff";
                        document.getElementById(key).querySelector(".hand-name").style.color = "#eaeaea";
                        document.getElementById(key).querySelector(".hand-value").style.color = "#eaeaea";
                    } else {
                        overlay.style.backgroundColor = "#000";
                        document.getElementById(key).querySelector(".hand-name").style.color = "#1a1a1a";
                        document.getElementById(key).querySelector(".hand-value").style.color = "#1a1a1a";
                    }
                }
            }
            document.getElementById("minval").textContent = minval.toFixed(3);
            document.getElementById("maxval").textContent = maxval.toFixed(3);
        })
}

function increment(button) {
    selector = document.getElementById("num-players")
    if (button.classList.contains("plus")) {
        if (selector.selectedIndex < selector.options.length - 1) {
            selector.selectedIndex++;
            submit(button);
        }
    } else {
        if (selector.selectedIndex > 0) {
            selector.selectedIndex--;
            submit(button);
        }
    }
}

document.addEventListener("DOMContentLoaded", function () {
    const tableBody = document.getElementById("table"); // tbody element

    // Create a 13x13 table for poker hands
    for (let i = 12; i >= 0; i--) {
        const row = document.createElement("tr");
        for (let j = 12; j >= 0; j--) {
            const cell = document.createElement("td");

            const drawOverlay = document.createElement("div");
            drawOverlay.classList.add("draw-overlay");
            cell.appendChild(drawOverlay);

            const cellName = document.createElement("div");
            if (j < i) {
                cellName.textContent = ranks[i] + ranks[j] + "s";
            } else if (j > i) {
                cellName.textContent = ranks[j] + ranks[i] + "o";
            } else {
                cellName.textContent = ranks[i] + ranks[j];
            }
            cellName.classList.add("hand-name");
            cell.appendChild(cellName);
            // set cell id for later updates
            cell.id = cellName.textContent;

            const valueDiv = document.createElement("div");
            valueDiv.classList.add("hand-value");
            valueDiv.textContent = "-";

            cell.appendChild(valueDiv);
            cell.addEventListener("mouseenter", function () {
                populate(this)
            });
            cell.addEventListener("click", function () {
                toggle(this)
            });

            row.appendChild(cell);
        }
        tableBody.appendChild(row);
    }

    // attach event listener to the button
    // document.getElementById("calculate").addEventListener("click", function () {
    //     submit(this);
    // });
    document.getElementById("num-players").addEventListener("change", function () {
        submit(this);
    });

    document.getElementById("table").addEventListener("mouseleave", function () {
        resetLegend();
    });

    window.addEventListener("click", function (e) {
        if (!e.target.closest("td")) {
            removeAllSelected();
            resetLegend();
        }
    });

    submit(this);
});
