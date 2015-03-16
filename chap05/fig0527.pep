;File: fig0527.pep
;Computer Systems, Fourth edition
;Figure 5.27
;
main:    DECI    exam1,d    ;cin >> exam1
         DECI    exam2,d    ;   >> exam2
         LDA     exam1,d    ;score = (exam1
         ADDA    exam2,d    ;   + exam2)
         ASRA               ;   / 2
         ADDA    bonus,i    ;   + bonus
         STA     score,d
         STRO    msg,d      ;cout << "score = "
         DECO    score,d    ;   << score
         CHARO   '\n',i     ;   << endl
         STOP
;
bonus:   .EQUATE 5          ;constant
exam1:   .BLOCK  2          ;global variable #2d
exam2:   .BLOCK  2          ;global variable #2d
score:   .BLOCK  2          ;global variable #2d
msg:     .ASCII  "score = \x00"
         .END
