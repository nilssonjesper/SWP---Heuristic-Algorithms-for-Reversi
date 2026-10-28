let str = `8
4
2
1
`;

// str is approximation
const len = str.split("\n").length;

// accuracy
let accuracy = str.split("\n").map((line, i) => {
    return 1 - ((line - len + i) / line);
});

// res is error
let res = str.split("\n").map((line, i) => {
    return line - len + i;
});

// entry, approximation, error, accuracy, average_error, average_accuracy
console.log(str.split("\n").map((line, i) =>
    (i + 1) + ' ' + line + ' ' + res[i] + ' ' + accuracy[i] + ' '
        + res.slice(i).reduce((a, c) => a + c, 0) / (res.length - i) + ' '
        + accuracy.slice(i).reduce((a, c) => a + c, 0) / (accuracy.length - i)).join("\n")
);
