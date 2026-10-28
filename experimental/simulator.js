const fs = require("fs");

const string = fs.readFileSync("./data.txt").toString();

const group = (string) => {
	switch (string) {
		case "The Stig": return 4;
		case "Beate": return 6;
		case "LaurAI": return 2;
		case "Phteven": return 3;
		case "Reversi++": return 5;
		case "PlayerOne": return 3;
		default: switch (string.slice(string.indexOf(" ") + 1)) {
			case "4": return 1;
			case "5": return 5;
			case "7": return 8;
			case "2": return 1;
			case "3": return 3;
			case "1": return 2;
			case "8": return 5;
			case "6": return 7;
			default: return string;
		}
	}
}

const special = (string) => {
	if (string == "" || string == "override" || string == "inversion") return "";
	if (string == "bonus bomb") return "20";
	if (string == "bonus override") return "21";

	if (string.includes("choosing")) return group(string.slice(string.indexOf(" ") + 1));

	return string;
}

const moves = string
	.split("group/")
	.reverse()
	.map(move => {
		if (move.includes("qualified"))
			return "disqualify " + group(move.substring(move.indexOf("\">") + 1, move.indexOf("</a")));

		const a = move.substring(move.indexOf("to"), move.indexOf("<", move.indexOf("to")));

		return group(move.substring(move.indexOf(">") + 1, move.indexOf("</a"))) + " " +
			a.substring(a.indexOf("(") + 1, a.indexOf(")")).replace(",", " ") +
			(a.includes("[") ? " " + special(a.substring(a.indexOf("[") + 1, a.indexOf("]"))) : "")
	})
	.slice(0, -1)
	.join("\n");

fs.writeFileSync("moves.txt", moves);

