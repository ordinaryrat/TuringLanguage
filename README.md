Executable is tur.\
Can run with ./tur file or do ./tur --help for options.\
\
Sample code is in ./sample.tur which capitalizes all letters of an input up to 200 bytes.\
\
This is a turing complete toy language that is intended to model the actions of a Turing Machine.\
Due to the way it interacts with syscalls this can technically do anything on the computer equivalent to what assembly can do.\
Probably not cross compatible outside of Linux, but haven't tested and it might be flexible enough.\
The compiler is written in C.

It has a tape of memory and then a tape pointer.
Each thing on the tape is a byte and can store from \x00 to \xFF.

Initial tape is:
```
\x02 \x03
       ^
```
Where ^ represents which byte the tape pointer is at.\
\x02 represents the start of the tape and \x03 represents the end.\
\x03 can be rewritten as more are allocated.\
Both of these can be overwritten in whatever way you want. I do not really recommend it though just because there isn't anything else that acts like a delimitor for the tape. Of course in a decent amount of situations that doesn't matter though.

Supported Data Types (converted to bytes):
- Unsigned Numbers,
- Bytes (0x10),
- Chars ('c'),
- Strings ("hello") -> Just syntactical sugar. In this case would set the byte at tape head to 'h' and then the next to 'e' and so on. Just more convenient than manually setting all bytes.\
        Also does the important little endian nature for you.\
        Example:\
              ```
              go 6
              set "hello"
              go 1
              set \x0a
              syscall 1 1 {6} 6
              syscall 60 0
              ```
              -> Will print 'hello\n'.
  
    - Dereferenced Pointers ([1]) -> Value is essentially just set to what is at memory address of 1.
        This can be used in operations like: add [2] which adds what is currently in [2] to the tape pointer byte.
    - Actual Pointers ({1}) -> Gives the actual memory address (8 bytes) of this address. This is probably only useful for certain syscalls that require buffers.

    Note that while individual bytes of memory cannot be set to numbers larger than 255, syscalls can contain those.

Commands:
+ alloc x -> Increases the free memory bounds by x. Essentially moves the \x03 further to the right. Note that the original \x03 will still be in the tape.
        Example: alloc 2
            Tape is now:
            \x02 \x03 \x00 \x03. 
                   ^
        The \x02 and \x03 can be overwritten but the ones at the ends can be useful probably.
        
        An equivalent is dealloc x which moves the \x03 to the left by x.

+ set x -> Sets memory of the currently looked at byte to x.
        Example: set 0x20
            Effect on previous tape:
                \x02 \x20 \x00 \x03
                       ^

+ go x -> Sets tape pointer to position x.
        Example: go 2
            Effect on previous tape:
                \x02 \x20 \x00 \x03
                            ^ 
            Note that 0-based indexing applies.
        
        gor and gol are equivalent relative movement commands.
        gor x -> Moves tape pointer x to the right.
        gol x -> Moves tape pointer x to the left.
    
+ add x, sub x -> Adds or subtracts tape pointing byte by x.
        Example: add 15
            Effect on previous tape:
                \x02 \x20 \x15 \x03.
                            ^
        For subtraction, two's complement would apply.
        Example: sub 30 
            Effect on previous tape:
                \x02 \x20 \xF1 \x03.
                            ^
    
 + syscall x1 x2 x3 x4 x5 x6 x7 -> Does a syscall with the following attributes set.
        x1 -> rax or what syscall.
        x2 - x7 represents the arguments. Technically more can be provided but will be ignored and a warning is given.
        For example:
            syscall 60 0 -> Exit syscall.
        Some syscalls want pointers to buffers. As previously mentioned the {x} operator gives the actual memory address of a byte in the tape.
        So like:
            syscall 1 1 {100} 10 -> Will write 10 bytes from a buffer starting at position 100.

        After every syscall the return value of it (rax) will be appended to the end of the tape.
        This is 8-bytes. 
        Effectively alloc 8 is done and then those 8 bytes are set to the value of rax.

Line comments are specified by //. Multi-line comments can be done with /* */.

if and while are both in this language.
if EXPR {

}
while EXPR {

}

Due to the nature of the language, recall that for a while loop it will check what the tape pointer is looking at before each run. So you can either move the tape pointer back to the iterator you want to use or use more creative approaches.

Expressions:
    This language features very basic expressions. 
    Essentially:
        [relation] value.
    Compares tape pointer byte to it via relation.
    For equal you do not need to specify a relation.

    These can be used for if and while loops.

    For instance:
        if 'c' {
            // Internal
        }
    Recall values also include Dereferenced pointers. You can do:
        if [2] {

        }

    Other relations that can be done:
        ! (not), < (less than), <= (less than or equal), > (greater than), >= (greater or equal).
        These ignore twos complement.

        Example:
            go 4
            if <='c' {
                // Done if the 5th byte on the tape is less than or equal to the byte value of 'c'.
            }

As mentioned previously, a sample code (for capitalizing an input string) is in ./sample.tur. This language is simple but is not easy.

More features will be added in the future potentially upon demand.
I could add arguments (as in running the program with arguments), calls to other code, and loading in code file upon demand (although it would take time).
Please report bugs to the github and I might fix them eventually. The source code is published there as well.

https://github.com/ordinaryrat/TuringLanguage
