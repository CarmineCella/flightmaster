unit U_WindTriangle2;
  {Copyright  © 2005, 2009  Gary Darby,  www.DelphiForFun.org
 This program may be used or modified for any non-commercial purpose
 so long as this original notice remains in place.
 All other rights are reserved
 }
interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ComCtrls, ExtCtrls, ShellAPI, U_ColorsDlg;

type
  TForm1 = class(TForm)
    PageControl1: TPageControl;
    TabSheet1: TTabSheet;
    TabSheet2: TTabSheet;
    PaintBox2: TPaintBox;
    MouseLbl: TLabel;
    Memo2: TMemo;
    GroupBox1: TGroupBox;
    HeadingTxt: TStaticText;
    GspeedTxt: TStaticText;
    WCATxt: TStaticText;
    WTATxt: TStaticText;
    StaticText3: TStaticText;
    StaticText4: TStaticText;
    StaticText5: TStaticText;
    StaticText6: TStaticText;
    WarningLbl: TLabel;
    GroupBox2: TGroupBox;
    AirSpeedUD: TUpDown;
    CourseUD: TUpDown;
    WindDirUD: TUpDown;
    WindSpeedUD: TUpDown;
    StaticText1: TStaticText;
    CWindDirTxt: TStaticText;
    StaticText7: TStaticText;
    Label5: TLabel;
    AirSpeedEdt: TLabeledEdit;
    CourseEdt: TLabeledEdit;
    WindSpeedEdt: TLabeledEdit;
    WindDirEdt: TLabeledEdit;
    Label1: TLabel;
    procedure EditChange(Sender: TObject);
    procedure PaintBoxPaint(Sender: TObject);
    procedure FormActivate(Sender: TObject);
    procedure StaticText1Click(Sender: TObject);
    procedure ColorChangeClick(Sender: TObject);
  public
    airspeed, windspeed, DesiredCourse, winddir:extended;
    WTAngle,wca, sinwca, groundspeed, heading:extended;
    trackV, WindV, HeadV:TPoint;  {vectors}
    procedure Calculate;
  end;

var
  Form1: TForm1;

implementation


{$R *.DFM}

uses math;

{************ Calculate **************}
procedure TForm1.Calculate;
{Calculate headiing and groundspeed based on given plane and wind parameters}
begin
  warninglbl.visible:=false;
  airspeed:=airspeedUD.position;
  windspeed:=windspeedUD.position;
  DesiredCourse:=Pi*courseUD.position/180; {keep angles in radians internally}
  WindDir:=Pi*WinddirUD.position/180+pi;  {wind direction corrected to TO direction}
  if winddir>pi then winddir:=winddir-2*pi;
  WTAngle:=(Desiredcourse-WindDir);
  if wtangle>pi then wtangle:=wtAngle-2*Pi;
  If WTangle<-Pi then WTAngle:=WTAngle+2*Pi;

  {By Sine Rule:  Sinwca/windspeed=sin(WTAngle)/airspeed}
  SinWca:=windspeed*sin(WTAngle)/airspeed; {Sin of the Wind correction angle}
  If abs(SinWca)<1 then
  begin
    wca:=arcsin(sinwca); {could also be -arcsin(wca) since sine of either is identical}
    heading:=DesiredCourse+wca;
    while heading>2*pi do heading:=heading-2*Pi;
    while heading<0 do heading:=heading+2*Pi;

    groundspeed:=airspeed*cos(wca)+windspeed*cos(WTAngle);
    //Cosine rule gives same result but more cumbersome
    //groundspeed:=sqrt(airspeed*airspeed+windspeed*windspeed-2*airspeed*windspeed*cos(pi-(wtAngle+wca)));
    begin
      CWindDirTxt.caption:=format(' "To" Wind Direction  %4.0f'+char($B0)+' ',[180*winddir/pi]);
      HeadingTxt.caption:=format(' Suggested Heading: %4.1f'+char($B0)+' ',[180*heading/pi]);
      GSpeedTxt.caption:=format(' Ground Speed: %4.1f ',[Groundspeed]);
      WTATxt.caption:=format('Wind to Track Angle: %4.1f'+char($B0)+' ',[180*WTAngle/pi]);
      WCATxt.caption:=format(' Wind Correction Angle:  %4.1f'+char($B0)+' ',[180*wca/pi]);
    end;
  end
  else
  begin
    Warninglbl.visible:=true;
  end;
  paintboxpaint(self);
end;


{************ FormActivate ************}
procedure TForm1.FormActivate(Sender: TObject);
begin
  SendMessage(Memo2.handle, EM_SETMARGINS, EC_LEFTMARGIN or EC_RIGHTMARGIN, (20 shl 16)+20);
  pagecontrol1.activepage:=TabSheet1;
end;

{********* EditChange *********}
procedure TForm1.EditChange(Sender: TObject);
{Replot wind triangle as inputs change}
begin
   if sender is TlabeledEdit then calculate;
   application.processmessages;
end;

var deg:char=char($b0); {degree symbol}

{**************** PaintBoxPaint **************}
procedure TForm1.PaintBoxPaint(Sender: TObject);
{repaint the Wind triangle image}
var
  cx,cy:integer;
  xw,yw,xg,yg,xh,yh:integer;
  scale:extended;
  s:string;
  N:integer;

        {*************** DrawArrow *********}
        procedure drawarrow(pb:TPaintbox; const cx,cy,tipx,tipy:integer);
        {draw an arrow from (cx,cy) to (tipx,tipy) }
        var
          dx,dy:extended;
          r,a, delta:extended;
        begin
          with pb, canvas do
          begin
            brush.color:=pen.color;
            moveto(cx,cy); lineto(tipx,tipy);
            dx:=tipx-cx; dy:=tipy-cy;
            a:=arctan2(dy,dx);

            r:=sqrt(sqr(dx)+sqr(dy))-8;
            if r>4 then delta:=arcsin(4/r) else delta:=0;
            polygon([point(cx+trunc(r*cos(a+delta)),cy+trunc(r*sin(a+delta))),
                   point(tipx,tipy),
                   point(cx+trunc(r*cos(a-delta)),cy+trunc(r*sin(a-delta)))]);
          end;
        end;

         {************** SetGrid *********}
        Procedure setgrid(pb:TPaintbox);
        {Initialize paintbox grid}
        begin
          with pb, canvas do
          begin
            cx:=width div 2; cy:=height div 2;
            scale:=0.9*cx/(airspeed+windspeed);
            color:=clwhite;
            brush.color:=color;
            pen.color:=clblack;
            pen.width:=1;
            rectangle(clientrect);
            moveto(cx,0); lineto(cx,height);
            moveto(0,cy); lineto(width,cy);
            s:='360'+deg; textout(cx-textwidth(s),2,s);
            s:='0'+deg; textout(cx+2,2,s);
            s:='90'+deg; textout(width-textwidth(s)-2,cy+2,s);
            s:='180'+deg; textout(cx+2,height-textheight(s)-2,s);
            s:='270'+deg; textout(2,cy+2,s);
          end;
        end;

        function dist(x,y:extended):extended;
        begin
          result:=sqrt(x*x+y*y);
        end;
begin {Paintboxpaint}
  with paintbox2, canvas do
  begin
    setgrid(paintbox2);

    {Course (track) line}
    pen.color:=CourseEdt.color; {clgreen}
    pen.width:=2;
    xg:=trunc(scale*groundspeed*sin(pi-desiredcourse));
    yg:=trunc(scale*groundspeed*cos(pi-desiredcourse));
    drawarrow(paintbox2,cx,cy, cx+xg, cy+yg);

    {Heading line}
    pen.color:=HeadingTxt.Color; {clBlue|}
    pen.width:=1;
    xh:=trunc(scale*airspeed*sin(pi-heading));
    yh:=trunc(scale*airspeed*cos(pi-heading));
    drawarrow(paintbox2,cx,cy, cx+xh,cy+yh);

    {Wind line}
    pen.color:=WindDirEdt.Color; {clred};
    xw:=trunc(scale*windspeed*sin(pi-winddir));
    yw:=trunc(scale*windspeed*cos(pi-winddir));
    drawarrow(paintbox2,cx+xg-xw,cy+yg-yw, cx+xg, cy+yg);

    {Wind To Track Angle}
     if abs(wca)>pi/60 then  {don't bother to draw the angle indicator if wca<5 degrees}
    begin
      pen.color:=WTATxt.Color; {cllime}
      pen.width:=2;

      N:=trunc(min(groundspeed*scale, windspeed*scale));
      if N<50 then N:=N div 2 else n:=25;
      if wTAngle<0 then arc(cx+xg-N,cy+yg-N,cx+xg+N,cy+yg+N,
                         cx+xg-xw, cy+yg-yw, cx, cy)
      else arc(cx+xg-N,cy+yg-N,cx+xg+N,cy+yg+N,
                        cx, cy, cx+xg-xw, cy+yg-yw);
    end;

    {wind correction angle}
    if abs(wca)>pi/60 then  {don't bother to draw the angle indicator if wca<5 degrees}
    begin
      pen.color:=WCATxt.Color; {clmaroon}
      pen.width:=2;

      N:=trunc(min(dist(xg,yg), dist(xh,yh)));
      if N<50 then N:=N div 2 else n:=25;
      if wca>0 then arc(cx-N,cy-N,cx+N,cy+N, cx+xh, cy+yh,cx+xg, cy+yg{, cx+xh,cy+yh})
      else arc(cx-N,cy-N,cx+N,cy+N, cx+xg, cy+yg,cx+xh, cy+yh{, cx+xg,cy+yg});
    end;
  end;
end;
(*
{ for debugging }
procedure TForm1.PaintBox2MouseMove(Sender: TObject; Shift: TShiftState; X,
  Y: Integer);
begin
   mouselbl.caption:=inttostr(x)+','+inttostr(y);
end;
*)

{************ ColorCHnage ****************}
procedure TForm1.ColorChangeClick(Sender: TObject);
{Let users change background, plot, and font colors}
var
  cap:string;
begin
  with u_colorsDlg.OKBottomDlg do
  begin
    if sender is TLabeledEdit then
    with sender as TLabeledEdit do
    begin
      bgcolor:=color;
      fontcolor:=font.color;
      If (sender = AirSpeedEdt)   then Cap:='Airspeed and Heading color'
      else If (sender=WindspeedEdt) or (sender=WindDirEdt)
           then Cap:='Wind speed and Wind Direction color'
      else if sender=CourseEdt then cap := 'Ground Speed and Course color';
    end
    else if sender is TStatictext then
    with sender as TStatictext do
    begin
      bgcolor:=color;
      fontcolor:=font.color;
      If (sender = CWindDirTxt)   then Cap:='Wind speed and Wind Direction color'
      else If (sender=WTATxt) then Cap:='Track to Wind Angle color'
      else if sender=WCATxt then cap := '* Wind Correction Angle color'
      else If (sender=HeadingTxt) then Cap:='Airspeed asnd Heading  color'
      else if sender=GSpeedTxt then cap := 'Ground Speed and Course color';
    end;
    caption:=cap;

    if showmodal=mrOK then
    begin
      Case caption[1] of
      'A':
        begin
          AirspeedEdt.color:=bgcolor;
          Airspeededt.Font.Color:=fontcolor;
          Headingtxt.Color:=bgcolor;
          HeadingTxt.Font.Color:=fontcolor;
        end;

      'W':
        begin
          WindspeedEdt.Color:=bgcolor;
          WindspeedEdt.font.color:=fontcolor;
          WindDirEdt.Color:=bgcolor;
          WindDirEdt.font.color:=fontcolor;
          CWinDDirtxt.Color:=bgcolor;
          CWinDDirtxt.font.color:=fontcolor;
        end;
      'G':
         begin
           CourseEdt.Color:=bgcolor;
           CourseEdt.font.color:=fontcolor;
           GSpeedTxt.Color:=bgcolor;
           GSpeedtxt.font.color:=fontcolor;
         end;
       'T':
         begin
           WTATxt.Color:=bgcolor;
           WTATxt.font.color:=fontcolor;
         end;
       '*':
         begin
           WCATxt.Color:=bgcolor;
           WCATxt.font.color:=fontcolor;
         end;

       end; {case}
       paintboxpaint(self);
    end;
  end;
end;

procedure TForm1.StaticText1Click(Sender: TObject);
begin
   ShellExecute(Handle, 'open', 'http://www.delphiforfun.org/',
  nil, nil, SW_SHOWNORMAL) ;
end;

end.


