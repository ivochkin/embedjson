function test_case {
}

echo "{}" | ./embedjson-parse

test_case "{}" "begin object\nend object"
