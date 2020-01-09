%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */

%x STR COMMENT

INTregExp [0-9]+
IDregExp [a-zA-Z][a-zA-Z0-9_]*

%%

<*> {
  "/*" {
    adjust(); 
    ++commentLevel_; 
    begin(StartCondition__::COMMENT);
  }
}

<COMMENT> {

  \n    {adjust(); errormsg.Newline();}

  "*/"  {
    adjust(); 
    --commentLevel_; 
    if (commentLevel_ == 0) {
      begin(StartCondition__::INITIAL);
    }
  }

  .     {adjust();}

}

/* String parsing -- adapted from the flex and flexc++ manuals */
<STR> {

  \" {
    adjustStr();
    if (stringBuf_.length() == 0) {
      setMatched("");
    } else {
      setMatched(stringBuf_);
    }
    begin(StartCondition__::INITIAL);
    return Parser::STRING;
  }

  \\([0-9]{3}) {
    /* octal escape sequence */
    adjustStr();
	  std::string oct_str = matched().substr(1);
	  int res = std::stoi(oct_str, nullptr, 10);
    char c = (char)res;

    if (res > 0xff) {
      errormsg.Error(errormsg.tokPos, "illegal character");
    }

    stringBuf_ += c;
  } /* character Ex. \101 'e' */

  \\[nt\"\\] {
	  /* Escape sequences for some special characters */
    adjustStr();
	  char c = matched().back(), res;

	  switch (c) {
	    case 'n':  res = '\n'; stringBuf_ += res; break;
	    case 't':  res = '\t'; stringBuf_ += res; break;
	    case '\"': res = '\"'; stringBuf_ += res; break;
	    case '\\': res = '\\'; stringBuf_ += res; break;
	  }
  }

  (\\\^)[\0-\37] {
    adjustStr();
    size_t esc_pos = matched().rfind('^');
	  std::string oct_str = matched().substr(esc_pos+1);
    int result = std::stoi(oct_str, nullptr, 8);
    stringBuf_ += (char)result;
  }
  
  (\\\^)[a-zA-Z] {
    adjustStr();
    char c = matched()[2];

    if (c >= '@' && c <= 'Z') {
      c -= 64;
    } else  {
      errormsg.Error(charPos_, "illegal character");
    }

    stringBuf_ += c;
  } 

  \\[ \t\n\r]+\\ {
    adjustStr();
    int i = 0;

    while (matched()[i] != '\0') {
      if (matched()[i] == '\n') {
        errormsg.Newline();
      }

      ++i;
    }
  }

  \\. {
    adjustStr();
    errormsg.Error(charPos_, "Illegal character");
  }

  \n  {
    adjustStr();
    errormsg.Newline();
    errormsg.Error(charPos_, "string terminated with newline");
  }

  [^\\\n\"] {
    adjustStr();
    char c = matched().back();
    stringBuf_ += c;
  }

  . {
    adjustStr();
    errormsg.Error(charPos_, "illegal char in string");
  }

}

<INITIAL> {

  (" "|"\t")  {adjust();} 
  \n	  {adjust(); errormsg.Newline();}
  ","	  {adjust(); return Parser::COMMA;}
  ":"   {adjust(); return Parser::COLON;}
  ";"   {adjust(); return Parser::SEMICOLON;}
  "("   {adjust(); return Parser::LPAREN;}
  ")"   {adjust(); return Parser::RPAREN;}
  "["   {adjust(); return Parser::LBRACK;}
  "]"   {adjust(); return Parser::RBRACK;}
  "{"   {adjust(); return Parser::LBRACE;}
  "}"   {adjust(); return Parser::RBRACE;}
  "."   {adjust(); return Parser::DOT;}
  "+"   {adjust(); return Parser::PLUS;}
  "-"   {adjust(); return Parser::MINUS;}
  "*"   {adjust(); return Parser::TIMES;}
  "/"   {adjust(); return Parser::DIVIDE;}

  "="   {adjust(); return Parser::EQ;}
  "<>"  {adjust(); return Parser::NEQ;}
  "<"   {adjust(); return Parser::LT;}
  "<="  {adjust(); return Parser::LE;}
  ">"   {adjust(); return Parser::GT;}
  ">="  {adjust(); return Parser::GE;}
  "&"   {adjust(); return Parser::AND;}
  "|"   {adjust(); return Parser::OR;}
  ":="  {adjust(); return Parser::ASSIGN;}
  array {adjust(); return Parser::ARRAY;}
  if    {adjust(); return Parser::IF;}
  then  {adjust(); return Parser::THEN;}
  else  {adjust(); return Parser::ELSE;}
  while {adjust(); return Parser::WHILE;}
  for   {adjust(); return Parser::FOR;}
  to    {adjust(); return Parser::TO;}
  do    {adjust(); return Parser::DO;}
  let   {adjust(); return Parser::LET;}
  in    {adjust(); return Parser::IN;}
  end   {adjust(); return Parser::END;}
  of    {adjust(); return Parser::OF;}
  break {adjust(); return Parser::BREAK;}
  nil   {adjust(); return Parser::NIL;}
  function  {adjust(); return Parser::FUNCTION;}
  var   {adjust(); return Parser::VAR;}
  type  {adjust(); return Parser::TYPE;}

  {INTregExp} {adjust(); return Parser::INT;}
  {IDregExp} {adjust(); return Parser::ID;}
  
  \" {
    adjust();
    stringBuf_.clear();
    begin(StartCondition__::STR);
  }
  
}

.	 {adjust(); errormsg.Error(errormsg.tokPos, "illegal token");}

%%