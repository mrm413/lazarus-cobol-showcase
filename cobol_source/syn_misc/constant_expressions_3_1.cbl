       IDENTIFICATION DIVISION.
       PROGRAM-ID. prog.
       DATA  DIVISION.
       WORKING-STORAGE SECTION.
       01  var       PIC X(200).
       78  var-len   value 115.

       PROCEDURE DIVISION.
       
           IF var-len < 16
              move notdefined to var *> won't be seen with -Wno-ignore-error
           END-IF

       >> IF SKIP-FIRST-ERROR NOT DEFINED
      *> always reported here (if active) as that's the first error
           move notdefined to var    
       >> END-IF

           IF var-len < 16
              move notdefined to var      *> never here, because of the warning/error above
           END-IF
           
           move var        to notdefined   *> only here if not the skip was active

           STOP RUN.
