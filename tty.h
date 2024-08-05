#ifndef  _TTY_H_
#define  _TTY_H_

char*  ctrls        (char c,  char* r) ;
err_t  tty_close    (void) ;
err_t  tty_setLnext (void) ; 
err_t  tty_showLit  (void) ;
err_t  tty_setCanon (void) ;
err_t  tty_getSig   (void) ;
err_t  tty_setSpeed (void) ;
err_t  tty_open     (char* tty) ;
err_t  tty_stuff    (char* s,  int len, int rat) ;
int    isban        (char ch) ;
int    isliteral    (char ch) ;

#endif //_TTY_H_
