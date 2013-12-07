### What is Eesk?
Eesk is an open source (GPL) programming language that attempts to approach freedom from a formal grammar by employing "safe-by-nature" interactions between its representation, abstraction, encapsulation, and evaluation mechanisms. Traditionally, programming languages protect the integrity of their runtime environment by enforcing strict syntactical or grammatic rules on its users. I suggest language can be more freely articulated if the atomic elements of language are safely associable in any context. Eesk intends to bypass the need for traditional "safe-by-rule" safety mechanisms, and therefore approach a minimal formal grammar, while remaining a platform for writing concise predictable code.

For a more exhaustive explaination and documentation of Eesk, please read the (early in progress) paper found in this repository: Eesk.pdf

The Eesk programming language is supported by the "ee" compiler and "eesk" loader found in the repository. This software has only been tested on x86-64 Ubuntu 12.04 (with regular updates).

### Begin Eesking
To install the language's supporting software, download the repository (by either using a link from above, or the first command below) and compile the source in the following way:
```
$ git clone https://github.com/theronrabe/eesk.git
$ cd <eesk>/compiler;make
$ cd ../vm; make
```
The included manual file contains a comprehensive documentation of Eesk.

### How to contribute to Eesk
Use the language. Try it for your next project. If things go smoothly, then keep using it. If not, contact me through github (@theronrabe), and I'll make it better for the next person. Have you done something to improve Eesk? Send a pull request!
