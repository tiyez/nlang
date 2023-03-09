# nlang

This is a transpiler from my language to C. The language is a cut version of C with AST macro support and build system commands in source file.

Syntax of language is C, but with inverted grammar of declarations. Also this language on purpose missing most control flow statements.
The only control flow here are 'if', 'else', 'while' and 'do while'. No 'continue', 'break', 'switch' and other. It's for code simplicity sake.

The language was designed for easy tooling. There was intention to write static analyzer for it. And for this structure of AST must be super simple.

There also I reflected on how to resolve code dependencies. I tried to make a simple system that allows a project to be a library for another project without
awareness. Each project has entry file that has #manifest declared with description of all source code and libraries used in this project. This entry file can
be included by other project in its own entry file as library or source file. The difference between library and source file is that library is exposing only
what written in manifest and source file is exposing everything into project namespace. And of course each library/project has own namespace.

This project is abandoned. No detailed explanation.
