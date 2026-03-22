       IDENTIFICATION DIVISION.
       PROGRAM-ID. my-prog.

       ENVIRONMENT DIVISION.
       CONFIGURATION SECTION.
       SOURCE-COMPUTER. GNU-Linux WITH DEBUGGING MODE.

       INPUT-OUTPUT SECTION.
       FILE-CONTROL.
           SELECT gen-file ASSIGN "f.txt".

       DATA DIVISION.
       FILE SECTION.
       FD  gen-file.
       01  gen-file-rec PIC X.

       PROCEDURE DIVISION.
           OPEN INPUT gen-file
           .
