%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%
%% gdv-class
%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Author:     Gerd Sussner <sussner@informatik.uni-erlangen.de>
%% Datum:      2001-10-20
%%
%% Stellt die Umgebungen 'gdvLetter', 'gdvSheet', 'figure', 'figure*', 'table'
%% und 'table*' bereit. Die letzten vier wurden von der 'article'-class kopiert
%% und stellen die vom 'article' bekannten Umgebungen dar.
%% 
%% Im folgenden werden die zusaetzlichen Umgebungen beschrieben.
%%
%% gdvLetter:
%%   Diese Umgebung setzt die Vorgabe der Technischen Fakultaet fuer ein ein-
%%   heitliches Layout der Briefkoepfe um. Das Argument wird als Betreff ge-
%%   wertet und in Fettschrift gesetzt.
%%
%%     \begin{gdvLetter}{<betreff>}{
%%       <body>
%%     \end{gdvLetter}
%%
%%   Makros:
%%     \Anrede{<text>}                Setzt <text> mit anschliessendem Komma und fuegt
%%                                    vertikalen Abstand von 1em ein
%%     \Gruss{<text>}{<mass>}         Fuegt vertikalen Abstand von <mass> ein und setzt
%%                                    <text> als Grussformel
%%     \Anlagen{<mass>}{<text>}       Setzt 'Anlagen' in Fettschrift, darunter dann <text>.
%%                                    Ist <mass> ein Leerstring, wird der Block linksbuending 
%%                                    zum Text gesetzt. Andernfalls um <mass> nach oben verschoben
%%                                    12cm Abstand vom linken Seitenrand gesetzt.
%%     \Verteiler{<mass>}{<text>}     Setzt 'Verteiler' in Fettschrift, darunter dann <text>
%%                                    (<mass> wie bei Anlagen)
%%     \PS{<text>}                    Setzt 'P.S.:' in Fettschrift. Anschliessend dann <text>
%%
%%
%% gdvSheet:
%%   Zeichnet die Uni- und das Lehrstuhllogo entsprechend der 'gdvLetter'-
%%   Umgebung. Abhaengig vom optionalem Parameter (default=true), wird der normale 
%%   Text mit einer duennen Linie vom Kopf getrennt.
%%   Bei einseitigen Dokumenten wird die Seitenanzeige auf der ersten
%%   Seite unterdrueckt.
%%
%%     \begin{gdvSheet}[drawline]{
%%       <body>
%%     \end{gdvSheet}
%%   
%%   Makros:
%%     \aufgabe{<nummer>}{<punkte>}{<zusatz>}  
%%
%%     \abgabevor{<limit>}
%%
%%     \abgabenach{<limit>}
%%
%%
%% Klassenoptionen:
%%   german    : deutsche Bezeichnungen, z.b. Datum
%%   english   : englische Bezeichnungen, z.b. Date
%%   intphone  : Vorwahl inkl. deutscher Vorwahl in der Form '(00 49 9131) '
%%   intphone* : Vorwahl inkl. deutscher Vorwahl in der Form '+49-9131-'
%%   faltmarken: Erzeuge Faltmarken
%%   farbig    : Verwende farbige Logos
%%   gradkoll  : Erzeuge Briefkopf fuer Graduiertenkolleg
%%
%% Makros, die vor der 'gdvLetter'-Umgebung angewendet werden koennen:
%%   Name              Variable              Default-Wert
%%   ==================================================================
%%   \Titel            \title                
%%   \Name             \name                 
%%   \Strasse          \strasse              Am Wolfsmantel 33
%%   \Ort              \ort                  91058 Erlangen
%%   \Vorwahl          \vorwahl              (0 91 31) 
%%   \Telefon          \telefon              85-
%%   \Durchwahl        \durchwahl            2 99 19
%%   \Faxdurchwahl     \faxdurchwahl         2 99 31
%%   \EMail            \email                
%%   \Unterschrift     \unterschrift         \name
%%   \Datum            \datum                aktuelles Datum in form jjjj-mm-tt
%%   \Aktenzeichen     \aktenzeichen         {}
%%   \Postvermerk      \postvermerk          {}
%%   \Versandart       \versandart           {}
%%   \Adresse          \adresse              {}
%%   \RetourAdresse    \retouradresse        Univ. Erlangen $\cdot$ Informatik 9 $\cdot$ \strasse $\cdot$ \ort
%%
%% \versandart und retouradresse werden mit \underline gesetzt
%%
%%
%%
%% Besondere Makros:
%%   \gdvDeckblatt     Erzeugt ein Deckblatt fuer Studien-/Diplomarbeiten
%%   
%%
%% Aenderungen:
%%   2004-05-07: Gerd Sussner <sussner@informatik.uni-erlangen.de>
%%               Hat der Uebergabeparameter fuer die Anzahl der Punkte beim
%%               Makro \aufgabe den Wert 0, wird die Ausgabe von '0 Punkte'
%%               unterdrueckt.
%%
%%   2004-06-15: Gerd Sussner <sussner@informatik.uni-erlangen.de>
%%               Die figure- und table-Umgebungen aus article.cls hinzugefuegt.
%%
%%   2007-06-27: Marco Winter <marco.winter@informatik.uni-erlangen.de>
%%               Alle Graphikelemente fuer die Fusszeile (TechFak / FAU) um
%%               10mm nach oben verschoben, damit neuer Drucker sie auch noch
%%               drucken kann.
%%
%%   2008-06-02: Marco Winter <marco.winter@informatik.uni-erlangen.de>
%%               Adresse geändert: Am Wolfsmantel 33
%% 
%%   2008-10-17: Marco Winter <marco.winter@informatik.uni-erlangen.de>
%%               Neues Logo eingebaut
%% 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\NeedsTeXFormat{LaTeX2e}[1996/06/01]
%%%%%%%%%%%%%%%%%%%%%%%%%%
%% benoetigte pakete laden
%%%%%%%%%%%%%%%%%%%%%%%%%%
\RequirePackage{ifthen}
\RequirePackage{palatino}
\RequirePackage{epsfig}
\RequirePackage{lastpage}


\ProvidesClass{gdv}[2001/10/20 GDV base class]


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Definition der figure and table Umgebungen (aus article.cls)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\newcounter{figure}
\renewcommand \thefigure {\@arabic\c@figure}
\def\fps@figure{tbp}
\def\ftype@figure{1}
\def\ext@figure{lof}
\def\fnum@figure{\figurename\nobreakspace\thefigure}
\newenvironment{figure}
               {\@float{figure}}
               {\end@float}
\newenvironment{figure*}
               {\@dblfloat{figure}}
               {\end@dblfloat}
\newcounter{table}
\renewcommand\thetable{\@arabic\c@table}
\def\fps@table{tbp}
\def\ftype@table{2}
\def\ext@table{lot}
\def\fnum@table{\tablename\nobreakspace\thetable}
\newenvironment{table}
               {\@float{table}}
               {\end@float}
\newenvironment{table*}
               {\@dblfloat{table}}
               {\end@dblfloat}
\newlength\abovecaptionskip
\newlength\belowcaptionskip
\setlength\abovecaptionskip{10\p@}
\setlength\belowcaptionskip{0\p@}
\long\def\@makecaption#1#2{%
  \vskip\abovecaptionskip
  \sbox\@tempboxa{#1: #2}%
  \ifdim \wd\@tempboxa >\hsize
    #1: #2\par
  \else
    \global \@minipagefalse
    \hb@xt@\hsize{\hfil\box\@tempboxa\hfil}%
  \fi
  \vskip\belowcaptionskip}


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Definition der Fonts von besonderen Textabschnitten
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Palatin (Ersatz fuer cmrm)
\newcommand{\PalatElevenDense}[1]{\fontsize{11}{12.8}\selectfont \textrm{#1} }
\newcommand{\PalatEleven}[1]{\fontsize{11}{13.2}\selectfont \textrm{#1} }
\newcommand{\PalatElevenRegular}[1]{\fontsize{11}{14}\selectfont \textrm{#1}}
%% Helvetica (Ersatz fuer cmss)
\newcommand{\HelvSeven}[1]{\fontsize{7}{8.4}\selectfont \textsf{#1} }
\newcommand{\HelvEight}[1]{\fontsize{8}{9.6}\selectfont \textsf{#1} }
\newcommand{\HelvTen}[1]{\fontsize{10}{12}\selectfont \textsf{#1} }
\newcommand{\HelvEleven}[1]{\fontsize{11}{13.2}\selectfont \textsf{#1} }
\newcommand{\HelvTwelve}[1]{\fontsize{12}{14.4}\selectfont \textsf{#1} }

%%%%%%%%%%%%%%%%%%%
%% LOGO-Verzeichnis
%%%%%%%%%%%%%%%%%%%
\def\logopathtf#1{gdv/LOGOS/#1}
\def\logopathlgdv#1{gdv/LOGOS/LGDV/#1}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Anpassung an lokale Drucker
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\newdimen\VorschubH
\newdimen\VorschubV
\VorschubH = 1.0mm
\VorschubV = 0.5mm

\def\defaulttitle{}
\def\defaultname{}
\def\defaultemail{baroti@informatik.uni-erlangen.de}
\def\defaultstrasse{Am Wolfsmantel 33}
\def\defaultort{91058 Erlangen}
\def\defaultuniversitaet{Univ.~Erlangen}
\def\defaultinstitut{Informatik~9}
\def\defaultvorwahl{(0 91 31) }
\def\defaultfaxdurchwahl{2 99 31}
\def\defaulttelefon{85-}
\def\defaultdurchwahl{2 99 19}

%\def\defaultGDVLogo#1{\epsfig{file=\logopath GDV/EPS/Logo_offiziell_bw2.eps,height=#1}}
%\def\defaultGDVLogoSmall#1{\epsfig{file=\logopath /GDV/EPS/Logo_offiziell_balken_bw.eps,height=#1}}
\def\defaultTFLogo{\epsfig{file=\logopathtf /Techfak/EPS/techfak-logo_bw.eps,height=6.5mm}}
\def\defaultTFText{\epsfig{file=\logopathtf /Techfak/EPS/techfaktxt_bw.eps,width=32mm}}
\def\defaultTFDeckblattLogo{\epsfig{file=\logopathtf /Techfak/EPS/techfak_komb_gr.eps,height=14mm}}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Deutsche Benennung fuer Fixfelder, z.b. \datumtext=Datum
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\DeclareOption{german}{
  \def\defaultGDVLogo#1{\epsfig{file=\logopathlgdv EPS/lgdv-de-bw.eps,height=#1}}
  \def\defaultGDVLogoSmall#1{\epsfig{file=\logopathlgdv EPS/lgdv-de-bw.eps,height=#1}}
  \def\sprache{german}
  \def\telefontext{Tel.\/:}
  \def\telefaxtext{Fax\/:}
  \def\emailtext{E--Mail\/:}
  \def\datumtext{Datum}
  \def\aktenzeichentext{Zeichen}
  \def\pageof{von}
  \def\anlagentext{\textbf{Anlagen}}
  \def\verteilertext{\textbf{Kopie(n) an}}
  \def\pstext{\textbf{P.S.\/:}}
  \def\aufgabenblatt{Aufgabenblatt\ }
  \def\institutsname{%
    Department Informatik\\%Institut f\"ur Informatik\\
%    \textbf{Lehrstuhl f\"ur Informatik 9}\\
    }%Graphische Datenverarbeitung\\}
  \def\institutsnamelength{143mm}
  \def\aufgabentext{Aufgabe}
  \def\punktetext{Punkte}
  \def\abgabetext{Abgabe}
  \def\abgabetextvor{{\em vor} der Vorlesung}
  \def\abgabetextnach{{\em nach} der Vorlesung}
}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Englische Benennung fuer Fixfelder, z.b. \datumtext=Date
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\DeclareOption{english}{
  \def\defaultGDVLogo#1{\epsfig{file=\logopathlgdv EPS/lgdv-en-bw.eps,height=#1}}
  \def\defaultGDVLogoSmall#1{\epsfig{file=\logopathlgdv EPS/lgdv-en-bw.eps,height=#1}}
  \def\sprache{english}
  \def\telefontext{Phone\/:}
  \def\telefaxtext{Fax\/:}
  \def\emailtext{E--Mail\/:}
  \def\datumtext{Date}
  \def\aktenzeichentext{Ref.}
  \def\pageof{of}
  \def\anlagentext{\textbf{encl.}}
  \def\verteilertext{\textbf{cc}}
  \def\pstext{\textbf{PS\/:}}
  \def\aufgabenblatt{exercise sheet\ }
  \def\institutsname{%
    Department of Computer Science\\
    %\textbf{Chair of Computer Science 9}\\%\textbf{Computer Graphics}\\
    }%Faculty of Engineering Sciences\\}
    \def\institutsnamelength{160.3mm}
  \def\aufgabentext{Assignment}
  \def\punktetext{Points}
  \def\abgabetext{Submission}
  \def\abgabetextvor{{\em before} class}
  \def\abgabetextnach{{\em after} class}
}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% internationale Vorwahl, alternative *-Form
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\DeclareOption{intphone}{
  \def\defaultvorwahl{(00 49 91 31) }
}
\DeclareOption{intphone*}{
  \def\defaultvorwahl{+49-9131-}
}


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% optionaler Graduiertenkolleg-Briefkopf
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\newboolean{gradkoll}
\setboolean{gradkoll}{false}
\DeclareOption{gradkoll}{
  \setboolean{gradkoll}{true}
}

%% Boolean fuer Trennlinie
\newboolean{separator}
\setboolean{separator}{true}

%% Erzeuge per Voreinstellung keine Faltmarken
\newboolean{faltmarken}
\setboolean{faltmarken}{false}
%%%%%%%%%%%%%%%%%%%%%
%% Zeichne Faltmarken
%%%%%%%%%%%%%%%%%%%%%
\DeclareOption{faltmarken}{
  \setboolean{faltmarken}{true}
}

%%%%%%%%%%%%%%%%%%%%%%%%%
%% Verwende farbige Logos
%%%%%%%%%%%%%%%%%%%%%%%%%
\DeclareOption{farbig}{
%  \def\defaultGDVLogo#1{\epsfig{file=\logopath /GDV/EPS/Logo_offiziell.eps,height=#1}}
  \def\defaultGDVLogo#1{\epsfig{file=\logopathlgdv EPS/lgdv-de-cl.eps,height=#1}}
%  \def\defaultGDVLogoSmall#1{\epsfig{file=\logopath /GDV/EPS/Logo_offiziell_balken.eps,height=#1}}
  \def\defaultGDVLogoSmall#1{\epsfig{file=\logopathlgdv EPS/lgdv-de-cl.eps,height=#1}}
  \def\defaultTFLogo{\epsfig{file=\logopathtf /Techfak/EPS/techfak-logo.eps,height=6.5mm}}
  \def\defaultTFText{\epsfig{file=\logopathtf /Techfak/EPS/techfaktxt.eps,width=32mm}}
  \def\defaultTFDeckblattLogo{\epsfig{file=\logopathtf /Techfak/EPS/techfak_komb.eps,height=14mm}}
}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Aktiviere 'german' als Defaultwert
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\ExecuteOptions{german} \ProcessOptions

%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Lade Basisklasse 'letter'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\LoadClass[a4paper, oneside, final]{letter}[1996/01/20]

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Standardfont (Palatino 11pt, Zeilenabstand 14pt)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\renewcommand{\normalsize}{\fontsize{11}{14}\selectfont}
\normalsize

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% alles auf den linken Seitenrand beziehen, d.h. den Offset aufheben,
%% und zusaetzlich noch 20mm Platz fuer Randnotizen lassen
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\setlength\hoffset{-20.4mm}          %% 5mm nicht bedruckbarer bereich
\setlength\voffset{-1in}
\setlength\oddsidemargin{19.1mm}     %% Text faengt bei 24.1mm an
\setlength\evensidemargin{\oddsidemargin}
\setlength\marginparwidth{90\p@}
\setlength\marginparsep{11\p@}
\setlength\marginparpush{5\p@}
\setlength\topmargin{0mm}
\setlength\headheight{35mm}
\setlength\headsep{0mm}
\setlength\topskip{0mm}
\addtolength\footskip{4.23mm}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Texthoehe setzen (bei der ersten Seite wird kuenstlich ein passender
%% vspace eingefuegt
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\setlength\textheight{245mm}
%\setlength\textwidth{170.9mm}
\setlength\textwidth{171.5mm}

\setlength\lineskip{1\p@}
\setlength\normallineskip{1\p@}

\renewcommand\baselinestretch{1}

%%%%%%%%%%%%%%%%%%%
%% Druckeranpassung
%%%%%%%%%%%%%%%%%%%
\advance\topmargin by \VorschubV
\advance\oddsidemargin by \VorschubH
\setlength\evensidemargin{\oddsidemargin}

\AtEndOfClass{\pagestyle{regularpage}}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Makros, die vor der gdvLetter-Umgebung verwendet werden koennen/muessen
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\def\Titel#1{\def\titel{#1}} \def\titel{\defaulttitle}
\def\Name#1{\def\name{#1}} \def\name{\defaultname}
\def\Strasse#1{\def\strasse{#1}} \def\strasse{\defaultstrasse}
\def\Ort#1{\def\ort{#1}} \def\ort{\defaultort}
\def\Adresse#1{\def\adresse{#1}} \def\adresse{}
\def\Datum#1{\def\datum{#1}} \def\datum{\number\year--\number\month--\number\day}
\def\Unterschrift#1{\def\unterschrift{#1}} \def\unterschrift{}
\def\RetourAdresse#1{\def\retouradresse{#1}} \def\retouradresse{}
\def\Vorwahl#1{\def\vorwahl{#1}} \def\vorwahl{\defaultvorwahl}
\def\Telefon#1{\def\telefon{#1}} \def\telefon{\defaulttelefon}
\def\Durchwahl#1{\def\durchwahl{#1}} \def\durchwahl{\defaultdurchwahl}
\def\Faxdurchwahl#1{\def\faxdurchwahl{#1}} \def\faxdurchwahl{\defaultfaxdurchwahl}
\def\EMail#1{\def\email{#1}} \def\email{\defaultemail}
\def\Aktenzeichen#1{\def\aktenzeichen{#1}} \def\aktenzeichen{}
\def\Postvermerk#1{\def\postvermerk{#1}} \def\postvermerk{}
\def\Versandart#1{\def\versandart{#1}} \def\versandart{}


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Makros, die in der gdvLetter-Umgebung verwendet werden koennen
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\def\Anrede#1{#1,\\[1em]}
\def\Gruss#1#2{\vspace{#1}\par\nobreak\stopbreaks\noindent%
  \parbox[t]{3.5in}{\raggedright \ignorespaces%
    {\normalsize%
      \ifthenelse{\equal{#1}{}}{\empty}{#2\mbox{}\\[16.92mm]}
      \ifthenelse{\equal{\unterschrift}{}}{\name}{\unterschrift}
    }
  }
  \par
}
\def\Anlagen#1#2{%
  \ifthenelse{\equal{#1}{}}{\vspace{2em}\par\parbox{\textwidth}{\anlagentext\\#2}\par}{\noindent\vspace{-#1}\par\hfill\parbox{77mm}{\anlagentext\\#2}\par}
}
\def\Verteiler#1#2{%
  \ifthenelse{\equal{#1}{}}{\vspace{-0.5em}\par\parbox{\textwidth}{\verteilertext\\#2}\par}{\noindent\vspace{-#1}\par\hfill\parbox{77mm}{\verteilertext\\#2}\par}
}
\def\PS#1{\noindent\vspace{-1em}\par\pstext\space#1\par}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Makros, die fuer Uebungsblaetter verwendet werden koennen
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\newcommand{\kopf}[4]{
  \vspace{-15mm}
  {\bf\sl \hspace{-11mm} #1 \hfill Erlangen, #2}
  \begin{center}
    {\large{\bf #3} (\aufgabenblatt #4)}
  \end{center}
}

\newcommand{\aufgabe}[3]{
  \vspace{1cm plus 5mm minus 5mm}%
  \pagebreak[3]%
  \hspace{-1cm}\textbf{\aufgabentext\space\space#1}
  \ifthenelse{\equal{#2}{0}}{\quad #3}{\quad[#2 \punktetext]\quad #3}
}

\newcommand{\abgabevor}[1]{
  \nopagebreak[4]%
  \begin{center}%
    \textbf{\abgabetext:\space#1, \abgabetextvor.}%
  \end{center}%
}

\newcommand{\abgabenach}[1]{
  \nopagebreak[4]%
  \begin{center}%
    \textbf{\abgabetext:\space#1, \abgabetextnach.}%
  \end{center}%
}

%%%%%%%%%%%%%%%%%
%% Deckblatt
%%%%%%%%%%%%%%%%%
\newcommand{\gdvDeckblatt}{
  \thispagestyle{firstpageDeckblatt}%
  \setlength\unitlength{1mm}%
  \c@page\@ne%
  \interlinepenalty=200%
  \clearpage%
  \vspace*{0mm}%
  \newpage
}

\newcommand{\logoposx}{0}
\newcommand{\institutposx}{-108.5}
\newcommand{\logoposy}{-33}
\newcommand{\institutposy}{-39}
%%%%%%%%%%%%%%%%%
%% Brief-Umgebung
%%%%%%%%%%%%%%%%%
\newenvironment{gdvLetter}[1]{
  \thispagestyle{firstpageLetter}
  \setlength\unitlength{1mm}

  \c@page\@ne
  \interlinepenalty=200
  \clearpage
  \vspace*{95mm}
  \textbf{#1}\par\vspace{1em}

	
}

%%%%%%%%%%%%%%%%%
%% Blatt-Umgebung
%%%%%%%%%%%%%%%%%
\newenvironment{gdvSheet}[1][true]{
  \setboolean{separator}{#1}
  \thispagestyle{firstpageSheet}%
  \setlength\unitlength{1mm}%
  \c@page\@ne%
  \interlinepenalty=200%
  \clearpage%
  \vspace*{35mm}%
  \ifthenelse{\boolean{separator}}{%
    \begin{picture}(0,0)%
      \put(-20.3,12) {\line(1,0){199}}%
    \end{picture}%
  }%
  {}%
}

 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Zeichnet Logos in einer Picture-Umgebung
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\def\gdvlogos{%
  %%%%%%%%%%%
  %% GDV-Logo
  %%%%%%%%%%%
  \put(\logoposx, \logoposy){%
  \raisebox{-2mm}{\defaultGDVLogo{15mm}}%
  }%
  %%%%%%%%%%%%
  %% GDV-Titel
  %%%%%%%%%%%%
  \put(\institutposx,  \institutposy){%
%     \parbox[b]{143mm}{%
    \parbox[b]{\institutsnamelength}{%
      \raggedleft\HelvTen{%
        \institutsname%
      }%
    }%
  }%
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  %% graue Balken der TechFak-FAU-Kombination
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  \put(-20.3,-270.5){%
    \epsfig{file=\logopathtf /Techfak/EPS/balken.eps,width=4mm,height=15.5mm}%
  }%
  \put(175,-287.2){%
    \epsfig{file=\logopathtf /Techfak/EPS/balken.eps,width=4mm,height=15.5mm}%
  }%
  %%%%%%%%%%%%%%%%%%%%%
  %% TechFak-Logo
  %%%%%%%%%%%%%%%%%%%%%
  \put(102.9,-280.){%
    \defaultTFLogo%
  }%
  %%%%%%%%%%%%%%%%%%%%%
  %% TechFak-Schriftzug
  %%%%%%%%%%%%%%%%%%%%%
  \put(115.3,-275.9){%
    \defaultTFText%
  }%
  %%%%%%%%%%%%%%%%%
  %% FAU-Schriftzug
  %%%%%%%%%%%%%%%%%
  \put(115.8,-271.2){%
    \epsfig{file=\logopathtf /FAU/EPS/fautxt.eps,width=51mm,height=7.8mm}%
  }%
  %%%%%%%%%%%
  %% FAU-Logo
  %%%%%%%%%%%
  \put(158.3,-281){%
    \epsfig{file=\logopathtf /FAU/EPS/fau-logo.eps,width=13mm,height=13mm}%
  }%
  \put(-16.3,-271.8) {\line(1,0){175.4}}%
  \put(170.5,-271.8) {\line(1,0){8.3}}%
  %%%%%%%%%%%
  %% Faltmarken
  %%%%%%%%%%%
  \ifthenelse{\boolean{faltmarken}}{%
    \put(-20,-149.6){%
      \epsfig{file=/home/gdv/tex/faltmarken.eps}%
    }%
  }%
  {}%
}%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Zeichnet Hilfslinien in einer Picture-Umgebung
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\def\hilfslinien{%
  \put(0,-35) {\line(1,0){200}}%
  \put(0,-117.5) {\line(1,0){200}}%
  \put(0,-90) {\line(1,0){76.9}}%
  \put(0,-46) {\line(1,0){200}}%
  \put(0,-50) {\line(1,0){200}}%
  \put(0,-52.5) {\line(1,0){200}}%
  \put(101.6,-200) {\line(0,1){200}}%
  \put(-19.3,-300) {\line(0,1){300}}%
  \put(0,-300) {\line(0,1){300}}%
  \put(2,-300) {\line(0,1){300}}%
  \put(171.9,-300) {\line(0,1){300}}%
  \put(0,-95) {\line(1,0){200}}%
  \put(0,-265) {\line(1,0){200}}%
  \put(-50,-105) {\line(1,0){200}}%
  \put(-50,-148.5) {\line(1,0){200}}%
  \put(0,-273.6) {\line(1,0){200}}%
  \put(0,-280.5) {\line(1,0){200}}%
  \put(0,-283.5) {\line(1,0){200}}%
  \put(0,-290) {\line(1,0){200}}%
  \put(102.9,-300) {\line(0,1){300}}%
  \put(115.9,-300) {\line(0,1){300}}%
  \put(184,-300) {\line(0,1){300}}%
}%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% einfaches Blatt, erste Seite mit Logo, folgende wie gdvLetter
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\def\ps@firstpageSheet{%
  \renewcommand{\baselinestretch}{1.0}%
  \ifcase \@ptsize\relax \normalsize \or \small \or \footnotesize \fi%
  \headheight0mm\headsep2.5mm%
  \def\@oddhead{%
    \unitlength1mm%
    
    \renewcommand\logoposx{-13.5}
    \renewcommand\institutposx{-121.7}
    \renewcommand\logoposy{-25.2}
    \renewcommand\institutposy{-31.2}
    
    \begin{picture}(0,0)%
      \gdvlogos%
      %%%%%%%%%%%%%%%%
      %% Seitenzaehler
      %%%%%%%%%%%%%%%%
      \put(128,-30) {%
        \ifthenelse{\pageref{LastPage}>1}{%
          \parbox{40mm}{%
            \PalatEleven{%
              \raggedleft\hfill\pagename\space\thepage\space\pageof\space\pageref{LastPage}\\%
            }%
          }%
        }%
        {}%
      }%
    \end{picture}%
    \hfill}%
  \def\@oddfoot{\empty}%
  \def\@evenhead{\@oddhead}%
  \def\@evenfoot{\@oddfoot}%
}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Deckblatt fuer Studien-/Diplomarbeiten
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\def\ps@firstpageDeckblatt{%
  \renewcommand{\baselinestretch}{1.0}%
  \ifcase \@ptsize\relax \normalsize \or \small \or \footnotesize \fi%
  \headheight0mm\headsep17.5mm%
  \def\@oddhead{%
    \unitlength1mm%
    \begin{picture}(0,0)%
      \put(50,-83){%
        \epsfig{file=/home/gdv/tex/fensterrahmen.eps,width=92mm,height=52mm}%
      }%
      %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
      %% graue Balken der TechFak-FAU-Kombination
      %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
      \put(44.5,-244.8){%
        \epsfig{file=\logopathtf /Techfak/EPS/balken.eps,width=5mm,height=70mm}%
      }%
      \put(141,-246){%
        \epsfig{file=\logopathtf /Techfak/EPS/balken.eps,width=5mm,height=-25mm}%
      }%
      %%%%%%%%%%%%%%%%%%%%%
      %% TechFak-Logo
      %%%%%%%%%%%%%%%%%%%%%
      \put(44.5,-262){%
        \defaultTFDeckblattLogo%
      }%
      %%%%%%%%%%%%%%%%%
      %% FAU-Schriftzug
      %%%%%%%%%%%%%%%%%
      \put(52,-245.7){%
        \epsfig{file=\logopathtf /FAU/EPS/fautxt.eps,width=87mm}%
      }%
      %%%%%%%%%%%
      %% FAU-Logo
      %%%%%%%%%%%
      \put(111,-261){%
        \epsfig{file=\logopathtf /FAU/EPS/fau-logo.eps,width=21mm,height=21mm}%
      }%
      \put(52,-246) {\thicklines\line(1,0){60}}%
      \put(131,-246) {\thicklines\line(1,0){14.9}}%
      %%%%%%%%%%
      %% Schloss
      %%%%%%%%%%
      \put(49,-220){%
        \epsfig{file=\logopathtf /FAU/EPS/Schloss.eps,width=95mm}%
      }%
      \put(148.5,-258){%
        \parbox{33mm}{%
          \HelvTen{Informatik 9}\\[0.5em]%
          \HelvEight{%
            \strasse\\%
            \ort\\%
            \makebox[6.1mm][l]{Tel.\/:}\vorwahl\telefon\durchwahl\\%
            \makebox[6.1mm][l]{Fax.\/:}\vorwahl\telefon\faxdurchwahl\\%
          }%
        }%
      }%
      \put(-15.5,-184){%
        \parbox{57mm}{%
          \raggedleft%
          \HelvTen{%
            Institut f\"ur Informatik\\%
            Technische Fakult\"at\\%
            Friedrich--Alexander--Universit\"at\\%
            Erlangen--N\"urnberg\\%
          }%
        }%
      }%
      \put(11.5,-227){%
        \defaultGDVLogo{12mm}%
      }%
      \put(-15.5,-239.5){%
        \parbox{57mm}{%
          \raggedleft%
          \HelvTen{%
            Lehrstuhl f\"ur Informatik 9\\%
            (Graphische Datenverarbeitung)\\%
            Prof. Dr. G\"unther Greiner\\%
          }%
        }%
      }%
    \end{picture}%
    \hfill}%
  \def\@oddfoot{\empty}%
  \def\@evenhead{\@oddhead}%
  \def\@evenfoot{\@oddfoot}%
}

%%%%%%%%%%%%%%%%
%% Briefumgebung
%%%%%%%%%%%%%%%%
\def\ps@firstpageLetter{%
  \renewcommand{\baselinestretch}{1.0}%
  \ifcase \@ptsize\relax \normalsize \or \small \or \footnotesize \fi%
  \headheight0mm\headsep17.5mm%
  \def\@oddhead{%
    \def\namestring{%
      \ifthenelse{\equal{\titel}{}}%
         {\name}%
         {\titel\space\name}%
    }%
    \def\telefonstring{\vorwahl\telefon\durchwahl}%
    \def\telefaxstring{\vorwahl\telefon\faxdurchwahl}%
    \def\retourstring{%
      \ifthenelse{\boolean{gradkoll}}%
        {\underline{Graduiertenkolleg\space $\cdot$ \strasse\space $\cdot$ \ort}}%
        {\ifthenelse{\equal{\retouradresse}{}}%
          {\underline{\defaultuniversitaet\space $\cdot$ \defaultinstitut\space $\cdot$ \strasse\space $\cdot$ \ort}}%
          {\underline{\retouradresse}}}%
    }%
    \unitlength1mm%
    
    \renewcommand\logoposx{101.8}
    \renewcommand\institutposx{-6.69}
    \begin{picture}(0,0)%
      %%%%%%%%
      %% Logos
      %%%%%%%%
      \gdvlogos%
      %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
      %% unterstrichene Ruecksende-Adresse
      %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
      \put(0.,-47.8){%
	\HelvSeven{\retourstring}%
      }%
      %%%%%%%%%%%%
      %% Anschrift
      %%%%%%%%%%%%
      \put(2,-52.7){%
	\parbox[t]{76.9mm}{%
          \PalatElevenDense{%
            \postvermerk\\%
            \underline{\versandart}\\%
            \adresse\\%
          }%
        }%
      }%
      %%%%%%%
      %% Name
      %%%%%%%
      \ifthenelse{\boolean{gradkoll}}%
      {%
        \put(101.4,-49){%
	  \parbox[t]{76mm}{\textbf{Graduiertenkolleg}\\%
                        \textsc{Dreidimensionale BildAnalyse\\ und -synthese}}
        }%
        \put(101.4,-70){%
          \makebox{\HelvTwelve{Sprecher: \namestring}}%
        }%
        %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        %% Adresse+Telefon/-fax+EMail
        %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        \put(101.8,-75.5){%
          \HelvEight{%
            \parbox[t]{76mm}{%
              \strasse\space $\cdot$ \ort\\%
              \begin{tabular}{p{10mm}l}%
                \hspace{-2.3mm}\telefontext & \telefonstring\\%
                \hspace{-2.3mm}\telefaxtext & \telefaxstring\\%
                \hspace{-2.3mm}\emailtext & \email%
              \end{tabular}%
            }%
          }%
        }%
      }%
      {%
        \put(101.4,-54){%
         \hfill\makebox{\HelvTwelve{\namestring}}%
        }%
        %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        %% Adresse+Telefon/-fax+EMail
        %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        \put(101.8,-59.5){%
          \HelvEight{%
            \parbox[t]{76mm}{%
              \strasse\space $\cdot$ \ort\\%
              \begin{tabular}{p{10mm}l}%
                \hspace{-2.3mm}\telefontext & \telefonstring\\%
                \hspace{-2.3mm}\telefaxtext & \telefaxstring\\%
                \hspace{-2.3mm}\emailtext & \email%
              \end{tabular}%
            }%
          }%
        }%
      }%
      %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
      %% Datum/Zeichen Ueberschrift
      %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
      \put(101.6,-97.) {%
        \HelvEight{\datumtext\ifthenelse{\equal{\aktenzeichentext}{}}{}{ / \aktenzeichentext}}%
      }%
      %%%%%%%%%%%%%%%%
      %% Datum/Aktenzeichen
      %%%%%%%%%%%%%%%%
      \put(101.7,-102.7) {%
        \PalatEleven{%
	  \makebox{\datum\ifthenelse{\equal{\aktenzeichentext}{}}{}{\ifthenelse{\equal{\aktenzeichen}{}}{}{\space /\space \aktenzeichen}}}%
        }%
      }%
      %%%%%%%%%%%%%%%%
      %% Seitenzaehler
      %%%%%%%%%%%%%%%%
      \put(129.7,-102.3) {%
        \parbox{40mm}{%
          \PalatEleven{%
            \raggedleft\hfill\pagename\space\thepage\space\pageof\space\pageref{LastPage}\\%
          }%
        }%
      }%
      %%%%%%%%%%%%%%
      %% Hilfslinien
      %%%%%%%%%%%%%%
      %\hilfslinien%
    \end{picture}%
    \hfill}%
  \def\@oddfoot{\empty}%
  \def\@evenhead{\@oddhead}%
  \def\@evenfoot{\@oddfoot}%
}

%%%%%%%%%%%%%%%%%%%%%%
%% normale Folgeseiten
%%%%%%%%%%%%%%%%%%%%%%
\def\ps@regularpage{%
  \def\@oddhead{\unitlength1mm%
    \begin{picture}(0,0)%
      \put(148.9,17){%
        \defaultGDVLogoSmall{10mm}%
      }%
      %%%%%%%%%%%%%%%%
      %% Seitenzaehler
      %%%%%%%%%%%%%%%%
      \put(132,12.5) {%
        \parbox{40mm}{%
          \PalatEleven{%
           \raggedleft\hfill\pagename\space\thepage\space\pageof\space\pageref{LastPage}\\%%
          }%%
        }%%
      }%%
      %%%%%%%%%%%
      %% Faltmarken
      %%%%%%%%%%%
      \ifthenelse{\boolean{faltmarken}}{%
        \put(-20,-114.6){%
          \epsfig{file=/home/gdv/tex/faltmarken.eps}%
        }%
      }%
      {}%
    \end{picture}%\hfill
  }%
  \def\@oddfoot{\empty}%
  \def\@evenhead{\@oddhead}%
  \def\@evenfoot{\@oddfoot}%
}

\def\istsprache{german}
\ifx \sprache\istsprache
\IfFileExists{babel.sty}
{\RequirePackage[\sprache]{babel}[1995/07/11] }
{\IfFileExists{german.sty}
  {\RequirePackage{german}[1996/05/01]}
  {\ClassError{gdv}
    {Neither babel nor german.sty installed !!!}
    {Get babel or german.sty !!!}}}
\else
\IfFileExists{babel.sty}
  {\RequirePackage[\sprache]{babel}[1995/07/11]}
  {\ClassError{gdv}
    {Babel not installed !!!}
    {Get babel package !!!}}
\fi

\endinput
%% 
%% End of file gdv.cls'.
