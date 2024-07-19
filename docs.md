# OrthoC Interpreter (occ)

An OrthoC program should have the following structure:

## Syntax

### 1. Prayer
- Syntax: `Prayer:`
- Purpose: Indicates the start of the program. If not found, the interpreter will remind the user to pray.

Example:
```
Prayer:
// Rest of the program follows
```



### 2. Functions
- Definition: `invoke functionName() { ... }`
- Calling: `call.upon functionName`

Examples:
```
invoke greet() {
    chant("Greetings, faithful one!");
}

call.upon greet
```



### 3. Variables
- Declaration: `incense variableName = "value";`

Example:
```
incense blessing = "Peace be with you";
```



### 4. Print Statement
- Syntax: `chant("text", variableName)`
- Prints text with optional variable interpolation

Examples:
```
chant("Hello, world!");
chant("Blessings to you, %s", name);
```



### 5. Loop
- Syntax: 'unceasingly.pray: FunctionName'
- Loops the process of a function

Example:
```
unceasingly.pray: FunctionName
```



### 6. Arithmetic
- Syntax: `theosis(2 + 2);`
- Purpose: Add ability to do mathematical equations

Example:
```
theosis(2 + 2);
theosis(2 + 2); > null   (<-- this will not output an answer)
```

TODO:
- Move to a transpiler (OrthoC -> C binary output) if im feeling adventerous
- Implementation of conditional statements (if/else)
- User input
- Basic file handling

