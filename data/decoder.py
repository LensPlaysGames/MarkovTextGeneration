import unicodedata;

with open("pushshift_reddit_r_suicidewatch.txt", "r", encoding='unicode-escape') as f:
    with open("output_decoded.txt", "w", encoding='ascii') as out:
        data = unicodedata.normalize('NFKD', f.read()).encode('ascii', 'ignore')
        print(data.decode('ascii'))
        out.write(data.decode('ascii'))
