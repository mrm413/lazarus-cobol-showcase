       IDENTIFICATION DIVISION.
       PROGRAM-ID. prog.
       DATA DIVISION.
       WORKING-STORAGE SECTION.
       01  num PIC 99 VALUE 0.
       PROCEDURE DIVISION.
           IF num LESS OR EQUALS 0
               CONTINUE
           END-IF
           IF num GREATER THAN OR EQUALS 1
               CONTINUE
           END-IF
           IF num EQUALS TO 0
               CONTINUE
           END-IF
           .
