TODO:

- To complete the parser:
  - [ ] Fix the correctness and completeness of the final tree structure
    - [ ] always stay in the current node instead of walking the ->next list each time. This may cause some issues with checking for ']' '}', need to look into solution of keeping track of depth as a sepparate variable which would be passed through gson_parse() function
  - [ ] Fix memory leaks (implement deletion of parsed nodes)
  - [ ] Handle special characters for strings (\" \\ \/ \b \f \n \r \t \u (0x01 0x02 0x03 0x04))
  - [ ] Implement check for UTF-8 characters
  - [ ] Write a bunch of tests
