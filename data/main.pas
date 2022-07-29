(* Depends on:   https://www.getlazarus.org/json/ *)
(* Compile like: `fpc main.pas`                   *)

program gather;

{$mode delphi}{$ifdef windows}{$apptype console}{$endif}

uses
crt,
jsontools,
fphttpclient,
opensslsockets,
sysutils;

var
   Node, NodeData, NodeIt : TJsonNode;
   JsonData               : ansistring;
   BaseURL                : string;
   DataURL                : string;
   SelfText               : string;
   Outfile                : TextFile;
   I                      : integer;

begin
   BaseURL := 'https://api.pushshift.io/reddit/search/submission/?subreddit=suicidewatch&fields=selftext&size=500';

   Assign(Outfile, 'pushshift_reddit_r_suicidewatch.txt');
   Rewrite(Outfile);

   Node := TJsonNode.Create;

   I := 5;
   while I < (365 * 3) do begin

      Delay(1000);
      DataURL := BaseURL + '&before=' + IntToStr(I) + 'd';
      jsondata := TFPHttpClient.SimpleGet(DataURL);

      Node.Clear;
      Node.Parse(jsondata);

      NodeData := Node.Find('data');

      for NodeIt in NodeData do begin
         if NodeIt.Exists('selftext') then begin
            SelfText := NodeIt.Find('selftext').Value;
            if Length(SelfText) <= 2  then continue;
            Delete(SelfText, 1, 1);
            Delete(SelfText, Length(SelfText), 1);
            if SelfText = '[removed]' then continue;
            if SelfText = '[deleted]' then continue;
            writeln(Outfile, SelfText);
         end;
      end;

      I := I + 3;

   end;

   Node.Free;

   Close(Outfile);

end.
