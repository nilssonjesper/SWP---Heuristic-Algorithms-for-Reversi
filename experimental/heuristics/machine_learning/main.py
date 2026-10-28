import matplotlib.pyplot as plt


# Die Result Datei lesen
def parse_file(filename):
    with open(filename, "r") as file:
        lines = file.readlines()

    segments = []
    i = 0
    while i < len(lines):
        if lines[i].startswith("Using:"):
            segment = {"using": [], "game": [], "players": []}
            using_line = lines[i].strip().split(":")[1].strip().split(" | ")
            segment["using"].append([float(x) for x in using_line])
            i += 1

            while i < len(lines) and "|" in lines[i]:
                segment["game"].append(
                    [float(x.strip()) for x in lines[i].strip().split("|")]
                )
                i += 1

            if i < len(lines) and "Game Result" in lines[i]:
                i += 1

            if i < len(lines):
                segment["players"].append(lines[i].strip())
                i += 1
            if i < len(lines):
                segment["players"].append(lines[i].strip())
                i += 1

            segments.append(segment)
        else:
            i += 1

    return segments


# Plot erstellen
def plot_segment(segment, index):

    weights = segment["using"][0]
    labels = [
        f"stones ({weights[0]})",
        f"walls ({weights[1]})",
        f"enemies ({weights[2]})",
        f"overrides ({weights[3]})",
        "score",
    ]
    data = list(zip(*segment["game"]))

    for i, col in enumerate(data):
        plt.plot(col, label=labels[i])

    def extract_score(line):
        import re

        if "disqualified" in line.lower():
            return 0
        match = re.search(r"(\d+)\s+points\b", line, re.IGNORECASE)
        if match:
            return int(match.group(1))
        return 0

    p1_score = extract_score(segment["players"][0])
    p2_score = extract_score(segment["players"][1])

    if p1_score == p2_score:
        plt.axhline(y=p1_score, color="black", linestyle=":")
    else:
        plt.axhline(y=p1_score, color="green", linestyle=":", label="Player 1 Score")
        plt.axhline(y=p2_score, color="red", linestyle=":", label="Player 2 Score")

    plt.title(f"Game {index + 21}")
    plt.xlabel("Moves")
    plt.ylabel("Evaluation")
    plt.legend(loc="upper left")
    plt.grid(True)
    plt.tight_layout()
    plt.show()


def main():
    filename = "/home/emy/Documents/Reversi/Results.txt"
    segments = parse_file(filename)
    for idx, segment in enumerate(segments):
        plot_segment(segment, idx)


if __name__ == "__main__":
    main()

