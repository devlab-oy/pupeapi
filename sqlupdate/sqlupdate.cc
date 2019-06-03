#include <sstream>
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <algorithm>

#undef SUPPORT_INSERTS

/*
 TODO: add support for CURRENT_TIMESTAMP , used with (DEFAULT,ON UPDATE)
*/

using namespace std;

#ifdef SUPPORT_INSERTS
static bool nodata = true;
#endif
static bool UseCreateIndex = false;
static bool AddComments = false;
static bool HandleCharsets = true;

static bool startwith(const char *s2, const string &s, size_t ind=0)
{
    size_t a;
    for(a=ind; s2 && *s2; ++a)
    {
        if(a >= s.size())return false;
        if(toupper(s[a]) != *s2++)
            return false;
    }
    return true;
}

static const string ucase(const string &s)
{
    string t;
    size_t a, b=s.size();
    for(a=0; a<b; ++a) t += (char)toupper(s[a]);
    return t;
}
static bool ischar(char c)
{
    return c=='_' || isalnum(c);
}
static const string GetWord(const string &s, size_t &a)
{
    // Upon return, a points 1 char after the value returned.
    
    string result;
    const size_t b = s.size();
    if(a < b && s[a] == '`')
    {
        ++a;
        while(a < b && s[a] != '`')
        {
            if(s[a] == '\\') ++a;
            result += s[a++];
        }
        if(a < b && s[a] == '`') ++a;
    }
    else
    {
        while(a < b && ischar(s[a]))
            result += s[a++];
    }
    return result;
}
static const string GetUcaseWord(const string &s, size_t &a)
{
    return ucase(GetWord(s, a));
}
static const std::string GetStringValue(const string& s, size_t& a, bool& was_quoted)
{
    std::string result = "";
    const size_t b = s.size();
    
    was_quoted = false;
    if(s[a]=='\'' || s[a]=='"')
    {
        char sep = s[a++];
        was_quoted = true;
        while(a<b)
        {
            if(s[a]==sep) { ++a; break; }
            if(s[a]=='\\')
            {
                ++a;
                switch(s[a])
                {
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    default: result += s[a]; // covers \\ and \" and \'
                }
            }
            else
                result += s[a];
            ++a;
        }
    }
    else
    {
        if(s[a] == '-' || s[a] == '+') result += s[a++];
        while(a<b && (isalnum(s[a]) || s[a]=='.' || s[a]=='_')) result += s[a++];
    }
    return result;
}
static const std::string QuoteWord(const std::string& s)
{
    // Quotes are ugly, so we only use them if necessary.
static const char* const ReservedWords[] =
{
"ADD","ALL","ALTER",
"ANALYZE","AND","AS",
"ASC","ASENSITIVE","BEFORE",
"BETWEEN","BIGINT","BINARY",
"BLOB","BOTH","BY",
"CALL","CASCADE","CASE",
"CHANGE","CHAR","CHARACTER","CHARSET",
"CHECK","COLLATE","COLUMN",
"CONDITION","CONNECTION","CONSTRAINT",
"CONTINUE","CONVERT","CREATE",
"CROSS","CURRENT_DATE","CURRENT_TIME",
"CURRENT_TIMESTAMP","CURRENT_USER","CURSOR",
"DATABASE","DATABASES","DAY_HOUR",
"DAY_MICROSECOND","DAY_MINUTE","DAY_SECOND",
"DEC","DECIMAL","DECLARE",
"DEFAULT","DELAYED","DELETE",
"DESC","DESCRIBE","DETERMINISTIC",
"DISTINCT","DISTINCTROW","DIV",
"DOUBLE","DROP","DUAL",
"EACH","ELSE","ELSEIF",
"ENCLOSED","ESCAPED","EXISTS",
"EXIT","EXPLAIN","FALSE",
"FETCH","FLOAT","FOR",
"FORCE","FOREIGN","FROM",
"FULLTEXT","GEOMETRY","GEOMETRYCOLLECTION",
"GOTO","GRANT",
"GROUP","HAVING","HIGH_PRIORITY",
"HOUR_MICROSECOND","HOUR_MINUTE","HOUR_SECOND",
"IF","IGNORE","IN",
"INDEX","INFILE","INNER",
"INOUT","INSENSITIVE","INSERT",
"INT","INTEGER","INTERVAL",
"INTO","IS","ITERATE",
"JOIN","KEY","KEYS",
"KILL","LEADING","LEAVE",
"LEFT","LIKE",
"LINESTRING",
"LIMIT",
"LINES","LOAD","LOCALTIME",
"LOCALTIMESTAMP","LOCK","LONG",
"LONGBLOB","LONGTEXT","LOOP",
"LOW_PRIORITY","MATCH","MEDIUMBLOB",
"MEDIUMINT","MEDIUMTEXT","MIDDLEINT",
"MINUTE_MICROSECOND","MINUTE_SECOND","MOD",
"MODIFIES",
"MULTILINESTRING","MULTIPOINT","MULTIPOLYGON",
"NATURAL","NOT",
"NO_WRITE_TO_BINLOG","NULL","NUMERIC",
"ON","OPTIMIZE","OPTION",
"OPTIONALLY","OR","ORDER",
"OUT","OUTER","OUTFILE",
"POINT","POLYGON",
"PRECISION","PRIMARY","PROCEDURE",
"PURGE","READ","READS",
"REAL","REFERENCES","REGEXP",
"RELEASE","RENAME","REPEAT",
"REPLACE","REQUIRE","RESTRICT",
"RETURN","REVOKE","RIGHT",
"RLIKE","SCHEMA","SCHEMAS",
"SECOND_MICROSECOND","SELECT","SENSITIVE",
"SEPARATOR","SET","SHOW",
"SMALLINT","SONAME","SPATIAL",
"SPECIFIC","SQL","SQLEXCEPTION",
"SQLSTATE","SQLWARNING","SQL_BIG_RESULT",
"SQL_CALC_FOUND_ROWS","SQL_SMALL_RESULT","SSL",
"STARTING","STRAIGHT_JOIN","TABLE",
"TERMINATED","THEN","TINYBLOB",
"TINYINT","TINYTEXT","TO",
"TRAILING","TRIGGER","TRUE",
"UNDO","UNION","UNIQUE",
"UNLOCK","UNSIGNED","UPDATE",
"USAGE","USE","USING",
"UTC_DATE","UTC_TIME","UTC_TIMESTAMP",
"VALUES","VARBINARY","VARCHAR",
"VARCHARACTER","VARYING","WHEN",
"WHERE","WHILE","WITH",
"WRITE","XOR","YEAR_MONTH",
"ZEROFILL"
};
const size_t nwords = sizeof(ReservedWords) / sizeof(*ReservedWords);
    
    bool needs_quoting = std::binary_search(ReservedWords, ReservedWords+nwords, ucase(s));
    
    if(!needs_quoting)
    {
        // Check for special characters
        for(size_t a=0; a<s.size(); ++a)
            if(!ischar(s[a])) { needs_quoting = true; break; }
    }
    if(needs_quoting)
    {
        std::string result = "`";
        for(size_t a=0; a<s.size(); ++a)
            if(s[a] == '`') result += "\\`";
            else result += s[a];
        result += '`';
        return result;
    }
    return s;
}
static const std::string GetDefaultCollate(const std::string& cset)
{
    static const struct{const char*cset; const char* collate; }
    SHOW_CHARSETS[] = {
{"ARMSCII8","ARMSCII8_GENERAL_CI" },
{"ASCII","ASCII_GENERAL_CI" },
{"BIG5","BIG5_CHINESE_CI" },
{"BINARY","BINARY" },
{"CP1250","CP1250_GENERAL_CI" },
{"CP1251","CP1251_GENERAL_CI" },
{"CP1256","CP1256_GENERAL_CI" },
{"CP1257","CP1257_GENERAL_CI" },
{"CP850","CP850_GENERAL_CI" },
{"CP852","CP852_GENERAL_CI" },
{"CP866","CP866_GENERAL_CI" },
{"CP932","CP932_JAPANESE_CI" },
{"DEC8","DEC8_SWEDISH_CI" },
{"EUCJPMS","EUCJPMS_JAPANESE_CI" },
{"EUCKR","EUCKR_KOREAN_CI" },
{"GB2312","GB2312_CHINESE_CI" },
{"GBK","GBK_CHINESE_CI" },
{"GEOSTD8","GEOSTD8_GENERAL_CI" },
{"GREEK","GREEK_GENERAL_CI" },
{"HEBREW","HEBREW_GENERAL_CI" },
{"HP8","HP8_ENGLISH_CI" },
{"KEYBCS2","KEYBCS2_GENERAL_CI" },
{"KOI8R","KOI8R_GENERAL_CI" },
{"KOI8U","KOI8U_GENERAL_CI" },
{"LATIN1","LATIN1_SWEDISH_CI" },
{"LATIN2","LATIN2_GENERAL_CI" },
{"LATIN5","LATIN5_TURKISH_CI" },
{"LATIN7","LATIN7_GENERAL_CI" },
{"MACCE","MACCE_GENERAL_CI" },
{"MACROMAN","MACROMAN_GENERAL_CI" },
{"SJIS","SJIS_JAPANESE_CI" },
{"SWE7","SWE7_SWEDISH_CI" },
{"TIS620","TIS620_THAI_CI" },
{"UCS2","UCS2_GENERAL_CI" },
{"UJIS","UJIS_JAPANESE_CI" },
{"UTF8","UTF8_GENERAL_CI" }
    };
    /* FIXME: use binary search */
    const size_t max = sizeof(SHOW_CHARSETS) / sizeof(*SHOW_CHARSETS);
    for(size_t a=0; a<max; ++a)
        if(cset == SHOW_CHARSETS[a].cset) return SHOW_CHARSETS[a].collate;
    return "";
}

static const std::string GetDefaultCharset(const std::string& coll)
{
    static const struct{const char*collate; const char* cset; }
    SHOW_COLLATES[] = {
{ "BIG5_CHINESE_CI","BIG5" },
{ "BIG5_BIN","BIG5" },
{ "DEC8_SWEDISH_CI","DEC8" },
{ "DEC8_BIN","DEC8" },
{ "CP850_GENERAL_CI","CP850" },
{ "CP850_BIN","CP850" },
{ "HP8_ENGLISH_CI","HP8" },
{ "HP8_BIN","HP8" },
{ "KOI8R_GENERAL_CI","KOI8R" },
{ "KOI8R_BIN","KOI8R" },
{ "LATIN1_GERMAN1_CI","LATIN1" },
{ "LATIN1_SWEDISH_CI","LATIN1" },
{ "LATIN1_DANISH_CI","LATIN1" },
{ "LATIN1_GERMAN2_CI","LATIN1" },
{ "LATIN1_BIN","LATIN1" },
{ "LATIN1_GENERAL_CI","LATIN1" },
{ "LATIN1_GENERAL_CS","LATIN1" },
{ "LATIN1_SPANISH_CI","LATIN1" },
{ "LATIN2_CZECH_CS","LATIN2" },
{ "LATIN2_GENERAL_CI","LATIN2" },
{ "LATIN2_HUNGARIAN_CI","LATIN2" },
{ "LATIN2_CROATIAN_CI","LATIN2" },
{ "LATIN2_BIN","LATIN2" },
{ "SWE7_SWEDISH_CI","SWE7" },
{ "SWE7_BIN","SWE7" },
{ "ASCII_GENERAL_CI","ASCII" },
{ "ASCII_BIN","ASCII" },
{ "UJIS_JAPANESE_CI","UJIS" },
{ "UJIS_BIN","UJIS" },
{ "SJIS_JAPANESE_CI","SJIS" },
{ "SJIS_BIN","SJIS" },
{ "HEBREW_GENERAL_CI","HEBREW" },
{ "HEBREW_BIN","HEBREW" },
{ "TIS620_THAI_CI","TIS620" },
{ "TIS620_BIN","TIS620" },
{ "EUCKR_KOREAN_CI","EUCKR" },
{ "EUCKR_BIN","EUCKR" },
{ "KOI8U_GENERAL_CI","KOI8U" },
{ "KOI8U_BIN","KOI8U" },
{ "GB2312_CHINESE_CI","GB2312" },
{ "GB2312_BIN","GB2312" },
{ "GREEK_GENERAL_CI","GREEK" },
{ "GREEK_BIN","GREEK" },
{ "CP1250_GENERAL_CI","CP1250" },
{ "CP1250_CZECH_CS","CP1250" },
{ "CP1250_CROATIAN_CI","CP1250" },
{ "CP1250_BIN","CP1250" },
{ "GBK_CHINESE_CI","GBK" },
{ "GBK_BIN","GBK" },
{ "LATIN5_TURKISH_CI","LATIN5" },
{ "LATIN5_BIN","LATIN5" },
{ "ARMSCII8_GENERAL_CI","ARMSCII8" },
{ "ARMSCII8_BIN","ARMSCII8" },
{ "UTF8_GENERAL_CI","UTF8" },
{ "UTF8_BIN","UTF8" },
{ "UTF8_UNICODE_CI","UTF8" },
{ "UTF8_ICELANDIC_CI","UTF8" },
{ "UTF8_LATVIAN_CI","UTF8" },
{ "UTF8_ROMANIAN_CI","UTF8" },
{ "UTF8_SLOVENIAN_CI","UTF8" },
{ "UTF8_POLISH_CI","UTF8" },
{ "UTF8_ESTONIAN_CI","UTF8" },
{ "UTF8_SPANISH_CI","UTF8" },
{ "UTF8_SWEDISH_CI","UTF8" },
{ "UTF8_TURKISH_CI","UTF8" },
{ "UTF8_CZECH_CI","UTF8" },
{ "UTF8_DANISH_CI","UTF8" },
{ "UTF8_LITHUANIAN_CI","UTF8" },
{ "UTF8_SLOVAK_CI","UTF8" },
{ "UTF8_SPANISH2_CI","UTF8" },
{ "UTF8_ROMAN_CI","UTF8" },
{ "UTF8_PERSIAN_CI","UTF8" },
{ "UTF8_ESPERANTO_CI","UTF8" },
{ "UTF8_HUNGARIAN_CI","UTF8" },
{ "UCS2_GENERAL_CI","UCS2" },
{ "UCS2_BIN","UCS2" },
{ "UCS2_UNICODE_CI","UCS2" },
{ "UCS2_ICELANDIC_CI","UCS2" },
{ "UCS2_LATVIAN_CI","UCS2" },
{ "UCS2_ROMANIAN_CI","UCS2" },
{ "UCS2_SLOVENIAN_CI","UCS2" },
{ "UCS2_POLISH_CI","UCS2" },
{ "UCS2_ESTONIAN_CI","UCS2" },
{ "UCS2_SPANISH_CI","UCS2" },
{ "UCS2_SWEDISH_CI","UCS2" },
{ "UCS2_TURKISH_CI","UCS2" },
{ "UCS2_CZECH_CI","UCS2" },
{ "UCS2_DANISH_CI","UCS2" },
{ "UCS2_LITHUANIAN_CI","UCS2" },
{ "UCS2_SLOVAK_CI","UCS2" },
{ "UCS2_SPANISH2_CI","UCS2" },
{ "UCS2_ROMAN_CI","UCS2" },
{ "UCS2_PERSIAN_CI","UCS2" },
{ "UCS2_ESPERANTO_CI","UCS2" },
{ "UCS2_HUNGARIAN_CI","UCS2" },
{ "CP866_GENERAL_CI","CP866" },
{ "CP866_BIN","CP866" },
{ "KEYBCS2_GENERAL_CI","KEYBCS2" },
{ "KEYBCS2_BIN","KEYBCS2" },
{ "MACCE_GENERAL_CI","MACCE" },
{ "MACCE_BIN","MACCE" },
{ "MACROMAN_GENERAL_CI","MACROMAN" },
{ "MACROMAN_BIN","MACROMAN" },
{ "CP852_GENERAL_CI","CP852" },
{ "CP852_BIN","CP852" },
{ "LATIN7_ESTONIAN_CS","LATIN7" },
{ "LATIN7_GENERAL_CI","LATIN7" },
{ "LATIN7_GENERAL_CS","LATIN7" },
{ "LATIN7_BIN","LATIN7" },
{ "CP1251_BULGARIAN_CI","CP1251" },
{ "CP1251_UKRAINIAN_CI","CP1251" },
{ "CP1251_BIN","CP1251" },
{ "CP1251_GENERAL_CI","CP1251" },
{ "CP1251_GENERAL_CS","CP1251" },
{ "CP1256_GENERAL_CI","CP1256" },
{ "CP1256_BIN","CP1256" },
{ "CP1257_LITHUANIAN_CI","CP1257" },
{ "CP1257_BIN","CP1257" },
{ "CP1257_GENERAL_CI","CP1257" },
{ "BINARY","BINARY" },
{ "GEOSTD8_GENERAL_CI","GEOSTD8" },
{ "GEOSTD8_BIN","GEOSTD8" },
{ "CP932_JAPANESE_CI","CP932" },
{ "CP932_BIN","CP932" },
{ "EUCJPMS_JAPANESE_CI","EUCJPMS" },
{ "EUCJPMS_BIN","EUCJPMS" }
    };
    /* FIXME: use binary search */
    const size_t max = sizeof(SHOW_COLLATES) / sizeof(*SHOW_COLLATES);
    for(size_t a=0; a<max; ++a)
        if(coll == SHOW_COLLATES[a].collate) return SHOW_COLLATES[a].cset;
    return "";
}


static bool isempty(char c)
{
    return c=='\n' || c=='\r' || c=='\t' || c==' ';
}
static void SkipBlank(const std::string& s, size_t& a)
{
    const size_t b = s.size();
    while(a < b && isempty(s[a])) ++a;
}

static const string sqlfix(const string &s, bool must_quote=false)
{
    if(!must_quote) // Check if it's an integral value, can omit quotes then
    {
        bool is_int_without_zerolead = true;
        if(s.empty()) is_int_without_zerolead = false;
        for(size_t zeroes=0, nonzeroes=0, a=0; a<s.size(); ++a)
        {
            if((s[a]<'0' || s[a]>'9')
            || (s[a]=='0' && zeroes>0 && nonzeroes==0))
            {
                is_int_without_zerolead = false;
                break;
            }
            if(!a && s[a]=='0') ++zeroes; else ++nonzeroes;
        }
        if(is_int_without_zerolead) return s;
    }

    string t;
    size_t a, b=s.size();
    for(a=0; a<b; ++a)
    {
        if(s[a]=='\\' || s[a]=='\'')t += '\\';
        t += s[a];
    }

	if(t.compare("CURRENT_TIMESTAMP")) return t;
    else return "'" + t + "'";
}

class fieldtype
{
    string type;
    size_t width, prec;
    string enumparams;
    bool autoincrement;
    bool zerofill;
    bool unsig;
    bool null;
    string deftext;
    string content;
    string collate,charset;
    bool binary, unique, fulltext, updatetimestamp;

    void RebuildContent()
    {
        content = type;
        if(type=="ENUM") { content += '('; content += enumparams; content += ')'; }
        if(width)
        {
            char Buf[64];
            content += '(';
            sprintf(Buf, "%u", (unsigned)width); content += Buf;
            if(prec) { sprintf(Buf, ",%u", (unsigned)prec); content += Buf; }
            content += ')';
        }
        content += ' ';
        if(zerofill)content += "ZEROFILL ";
        if(unsig)content += "UNSIGNED ";
        if(binary)content += "BINARY ";
        if(unique)content += "UNIQUE ";
        if(HandleCharsets)
        {
            if(!charset.empty()) content += "CHARSET " + charset + " ";
            if(!collate.empty()) content += "COLLATE " + collate + " ";
        }
        if(fulltext)content += "FULLTEXT ";
        if(null)content += "NULL "; else { content += "NOT NULL "; forcedefault(); }
        if(autoincrement)
        {
            content += "AUTO_INCREMENT ";
        }
		if(updatetimestamp)
		{
			content += "ON UPDATE CURRENT_TIMESTAMP ";
		}
    }

    void FixCharset()
    {
        if(!HandleCharsets)
        {
            charset = collate = "";
            return;
        }
        if(!charset.empty() && collate.empty()) collate = GetDefaultCollate(charset);
        if(charset.empty() && !collate.empty()) charset = GetDefaultCharset(collate);
    }
public:
    inline bool operator!= (const fieldtype &s) const { return !operator==(s); }
    bool operator== (const fieldtype &s) const
    {
        return s.width==width
           &&  s.prec==prec
           &&  s.autoincrement==autoincrement
           &&  s.zerofill==zerofill
           &&  s.unsig==unsig
           &&  s.null==null
           &&  s.type==type
           &&  s.binary==binary
           &&  s.charset==charset
           &&  s.collate==collate
           &&  s.updatetimestamp==updatetimestamp;
    }
    const char *c_str() const { return content.c_str(); }
    const string& defval() const { return deftext; }
    void forcedefault()
    {
        if(!deftext.empty()) return;
        if(autoincrement) return;
		if(updatetimestamp) return;
        
        if(type=="INT" || type=="TINYINT" || type=="SMALLINT" || type=="BIGINT")
        {
            deftext = sqlfix("0");
        }
        else if(type=="DECIMAL" || type=="DOUBLE" || type=="FLOAT")
        {
            deftext = "0";
            if(prec) deftext += '.';
            for(size_t a=0; a<prec; ++a) deftext += '0';
            deftext = sqlfix(deftext);
        }
        else if(type=="ENUM")
            {} // no default
        else
            deftext = sqlfix("");
    }
    const string& gettype() const { return type; }
    const string& getcollate() const { return collate; }
    const string& getcharset() const { return charset; }
    size_t getwidth() const { return width; }
    void settype(const string &t) { type=t; RebuildContent(); }
    void setcharset(const string &t) { charset=t; FixCharset(); RebuildContent(); }
    void setcollate(const string &t) { collate=t; FixCharset(); RebuildContent(); }
    
    void clearcharsetandcollate() { charset=collate=""; RebuildContent(); }

    explicit fieldtype(const string &s) :
        type(),
        width(0), prec(0),
        enumparams(),
        autoincrement(false),
        zerofill(false), unsig(false), null(true),
        deftext(), content(s),
        collate(), charset(),
        binary(false), unique(false), fulltext(false), updatetimestamp(false)
    {
        size_t b=s.size(), c=0;
        SkipBlank(s, c);
        size_t c_before = c;
        type = GetUcaseWord(s, c);
        if(type.empty())
        {
            throw("Can't get type from '" + s.substr(c_before) + "'");
        }
        
        // Resolve aliases
        if(type=="INTEGER")type="INT";
        else if(type=="REAL")type="DOUBLE";
        else if(type=="NUMERIC")type="DECIMAL";

        // Resolve default parameters AND bail out if unknown types
        if(type=="INT")width=11;
        else if(type=="SMALLINT")width=6;
        else if(type=="TINYINT")width=4;
        else if(type=="MEDIUMINT")width=9;
        else if(type=="BIGINT")width=20;
        else if(type=="DOUBLE")width=16, prec=4;
        else if(type=="FLOAT")width=10, prec=2;
        else if(type=="TIMESTAMP")width=14;
        else if(type=="DECIMAL"){}

        else if(type=="CHAR")width=1;
        else if(type=="VARCHAR"){}
        else if(type=="TEXT"){}
        else if(type=="MEDIUMTEXT"){}
        else if(type=="LONGTEXT"){}
        else if(type=="BLOB"){}
        else if(type=="MEDIUMBLOB"){}
        else if(type=="LONGBLOB"){}
        else if(type=="DATETIME"){}
        else if(type=="DATE"){}
        else if(type=="TIME"){}
        else if(type=="ENUM"){}

        // Spatial datatypes
        else if(type=="GEOMETRY"){}
        else if(type=="POLYGON"){}
        else if(type=="POINT"){}
        else if(type=="LINESTRING"){}
        else if(type=="MULTIPOINT"){}
        else if(type=="MULTILINESTRING"){}
        else if(type=="MULTIPOLYGON"){}
        else if(type=="GEOMETRYCOLLECTION"){}
        
        else
        {
            fprintf(stderr, "Warning: Unknown type '%s'\n", type.c_str());
        }
        SkipBlank(s, c);
        if(c<b)
        {
            // Parse parameters of the type
            if(s[c]=='(')
            {
                ++c; SkipBlank(s, c);
                if(type == "ENUM")
                {
                    while(c<b)
                    {
                        SkipBlank(s, c);
                        bool was_quoted=false;
                        enumparams += sqlfix(GetStringValue(s, c, was_quoted),true);
                        if(!was_quoted)
                            fprintf(stderr, "Syntax error in ENUM params: ENUM values must be quoted.");
                        SkipBlank(s, c);
                        if(c<b && s[c]==',') { enumparams += s[c++]; continue; }
                        break;
                    }
                }
                else
                {
                    for(width=0; c<b && isdigit(s[c]); ++c)width=width*10+s[c]-'0';
                    SkipBlank(s, c);
                    if(c<b && s[c]==',')
                    {
                        ++c; SkipBlank(s, c);
                        for(prec=0; c<b && isdigit(s[c]); ++c)prec=prec*10+s[c]-'0';
                        SkipBlank(s, c);
                    }
                }
                if(c>=b || s[c]!=')')
                {
                    throw("Expected ')' in '" + s + "'");
                }
                ++c;
            }
            
            // Parse the parameters of the type
            while(c<b)
            {
                string t;
                SkipBlank(s, c);
                //fprintf(stderr, "Analyzing '%s'...\n", s.substr(c).c_str());
                if(c>=b)break;
                t = GetUcaseWord(s, c);
                if(t.empty())
                {
                    throw("Can't get a word from '" + s + "' at '" + s.substr(c) + "'");
                }
                if(t=="AUTO_INCREMENT")autoincrement = true;
                else if(t=="UNSIGNED")unsig = true;
                else if(t=="BINARY")binary = true;
                else if(t=="UNIQUE")unique = true;
                else if(t=="ZEROFILL")zerofill = true;
                else if(t=="FULLTEXT")fulltext = true;
                else if(t=="NULL")null = true;
                else if(t=="PRIMARY")
                {
                    SkipBlank(s, c);
                    t = GetUcaseWord(s, c);
                    if(t!="KEY")throw("PRIMARY "+t+"? KEY expected.");
                    /* ignore */
                }
                else if(t=="NOT")
                {
                    SkipBlank(s, c);
                    t = GetUcaseWord(s, c);
                    if(t!="NULL")throw("NOT "+t+"? NULL expected.");
                    null = false;
                }
                else if(t=="DEFAULT")
                {
                    SkipBlank(s, c);
                    bool was_quoted=false;
                    deftext = GetStringValue(s, c, was_quoted);
                    if(!was_quoted && deftext == "NULL")
                        deftext = "";
                    else
                        deftext = sqlfix(deftext);
                }
                else if(t=="CHARSET")
                {
                GotCharSet:
                    SkipBlank(s, c);
                    bool was_quoted=false;
                    charset = ucase(GetStringValue(s, c, was_quoted));
                }
                else if(t=="CHARACTER")
                {
                    SkipBlank(s,c);
                    std::string word = GetUcaseWord(s,c);
                    if(word == "SET") goto GotCharSet;
                }
                else if(t=="COLLATE")
                {
                    SkipBlank(s, c);
                    bool was_quoted=false;
                    collate = ucase(GetStringValue(s, c, was_quoted));
                }
				else if(t=="ON")
				{
					SkipBlank(s, c);
					t = GetUcaseWord(s, c);
					if(t!="UPDATE")throw("ON "+t+"? UPDATE expected.");
					SkipBlank(s, c);
					t = GetUcaseWord(s, c);
					if(t!="CURRENT_TIMESTAMP")throw("UPDATE "+t+"? CURRENT_TIMESTAMP expected.");
					else updatetimestamp = true;
				}
                else
                    throw("What is '" + t + "' of '" + s + "'? Type is '" + type + "'");
            }
            //fprintf(stderr, "(Done)\n");
        }
        
        // Discard defaults and rename the type if necessary.
                
        if(type=="INT") { if(width==11)width=0; }
        else if(type=="BIGINT") { if(width==20)width=0; }
        else if(type=="TINYINT") { if(width==4)width=0; }
        else if(type=="SMALLINT") { if(width==6)width=0; }
        else if(type=="DOUBLE") { if(width==16 && prec==4)width=prec=0; }
        else if(type=="FLOAT") { if(width==10 && prec==2)width=prec=0; }
        else if(type=="CHAR") { if(width==1)width=0; }
        else if(type=="VARCHAR" && width<4)type="CHAR";
        else if(type=="DATETIME" && deftext.empty()) deftext=sqlfix("0000-00-00 00:00:00");
        else if(type=="DATE" && deftext.empty()) deftext=sqlfix("0000-00-00");
        else if(type=="TIME" && deftext.empty()) deftext=sqlfix("00:00:00");
        else if(type=="TIMESTAMP") { width=(width+1)&~1; if(width>=14)width=0; }
        
        FixCharset();
        RebuildContent();
    }
};
struct ForeignKeyData
{
    string ConstraintName;
    string IndexFields;
    string ReferenceTable;
    string ReferenceFields;
    string OnDelete;
    string OnUpdate;
    
    ForeignKeyData()
        : ConstraintName(), IndexFields(),
          ReferenceTable(), ReferenceFields(),
          OnDelete(), OnUpdate()
    {
    }
    
    const std::string MakeString() const
    {
        std::string result;
        result = "(" + IndexFields + ")REFERENCES "+QuoteWord(ReferenceTable)
               + "(" + ReferenceFields + ")";
        if(!OnDelete.empty()) result += "ON DELETE " + OnDelete + " ";
        if(!OnUpdate.empty()) result += "ON UPDATE " + OnUpdate + " ";
        return result;
    }
    
    bool operator!= (const ForeignKeyData& b) const { return !(*this == b); }
    bool operator== (const ForeignKeyData& b) const
    {
        return IndexFields    == b.IndexFields
            && ReferenceTable == b.ReferenceTable
            && ReferenceFields == b.ReferenceFields
            && OnDelete == b.OnDelete
            && OnUpdate == b.OnUpdate;
    }
};

struct SimulateFieldOrder
{
    std::list<std::string> fields;
public:
    SimulateFieldOrder() : fields() {}

    void Add(const std::string& s)
    {
        fields.push_back(s);
    }
    void Drop(const std::string& s)
    {
        std::list<std::string>::iterator i;
        for(i=fields.begin(); i!=fields.end(); ++i)
            if(*i == s) { fields.erase(i); break; }
    }
    void Add_After(const std::string& prev, const std::string& s)
    {
        if(prev.empty()) { fields.push_front(s); return; }
        
        std::list<std::string>::iterator i;
        for(i=fields.begin(); i!=fields.end(); ++i)
            if(*i==prev)
            {
                ++i;
                fields.insert(i, s);
                return;
            }        
        fields.push_back(s);
    }
    void Move_After(const std::string& prev, const std::string& s)
    {
        Drop(s);
        Add_After(prev, s);
    }
    const std::string FindPrecedent(const std::string& s) const
    {
        std::string result = "";
        std::list<std::string>::const_iterator i;
        for(i=fields.begin(); i!=fields.end(); ++i)
        {
            if(*i == s) break;
            result = *i;
        }
        return result;
    }
};

typedef pair<string, fieldtype> fieldpairtype;
typedef vector<fieldpairtype> fieldlisttype;
typedef pair<string, string> keypairtype;
typedef vector<keypairtype> keylisttype;
typedef pair<string, ForeignKeyData> foreignkeypairtype;
typedef vector<foreignkeypairtype> foreignkeylisttype;

string database = "winnie3";

class InputFailedError { public: bool pip; InputFailedError(bool p) : pip(p) { } };

static bool CaseEqual(const string &a, const string &b)
{
    size_t aasi = a.size();
    size_t beesi= b.size();
    if(aasi != beesi)return false;
    for(size_t p=0; p<aasi; ++p)
        if(toupper(a[p]) != toupper(b[p]))
            return false;
    return true;
}

class Table
{
    string nam;
    string typ;
    string primarykey;
    string primarykeytype;
    string charset;
    string collate;
    
public:
    fieldlisttype fields;
    keylisttype keys;
    foreignkeylisttype fks;
    
#ifdef SUPPORT_INSERTS
    vector<string> inserts;
#endif
    
    Table(const string &name, const std::string& type="")
        : nam(name), typ(type),
          primarykey(), primarykeytype(),
          charset(), collate(),
          fields(), keys(), fks()
#ifdef SUPPORT_INSERTS
          , inserts()
#endif
    {
    }
    void settype(const std::string& p) { typ = p; }
    void setcharset(const std::string& p) { charset=p; }
    void setcollate(const std::string& p) { collate=p; }
    void add(const string &name, const string &type)
    {
        //fprintf(stderr, "*** Table '%s': '%s' = '%s'\n", nam.c_str(), name.c_str(), type.c_str());
        fields.push_back(fieldpairtype(name, fieldtype(type)));
    }
    void addkey(const string &name, const string &type)
    {
        keys.push_back(keypairtype(name, type));
    }
    void addforeign(const string& indexcols, const ForeignKeyData& details)
    {
        fks.push_back(foreignkeypairtype(indexcols, details));
    }
    void setprimary(const string &n, const string& indextype)
    {
        primarykey = n;
        primarykeytype = indextype;
        for(;;)
        {
            string::size_type a = primarykey.find(", ");
            if(a==primarykey.npos)break;
            primarykey.erase(a+1, 1); // convert ", " into ","
        }
        // fprintf(stderr, "*** Table '%s': primary = '%s'\n", nam.c_str(), n.c_str());
    }
    inline const string &name() const { return nam; }
    inline const string &type() const { return typ; }
    inline const string &pkey() const { return primarykey; }
    inline const string &pkeytype() const { return primarykeytype; }
    bool hasfield(const string &n) const
    {
        size_t a, b=fields.size();
        for(a=0; a<b; ++a)
            if(CaseEqual(fields[a].first, n))return true;
        return false;
    }
    const fieldpairtype &getfield(const string &n) const
    {
        size_t a, b=fields.size();
        for(a=0; a<b; ++a)
            if(CaseEqual(fields[a].first, n))
                return fields[a];
        return fields[0]; // error
    }
    
    const string getkey(const string &search) const
    {
        size_t a, b=keys.size();
        for(a=0; a<b; ++a)
            if(CaseEqual(keys[a].first, search))
                return keys[a].second;
        return "";
    }
    const ForeignKeyData getfk(const string &search) const
    {
        size_t a, b=fks.size();
        for(a=0; a<b; ++a)
            if(CaseEqual(fks[a].first, search))
                return fks[a].second;
        return ForeignKeyData();
    }
#ifdef SUPPORT_INSERTS
    void Insert(const string &s)
    {
        inserts.push_back(s);
    }
    void FlushInserts() const
    {
        size_t a, b=inserts.size();
        for(a=0; a<b; ++a)
            printf("insert into %s %s;\n", nam.c_str(), inserts[a].c_str());
    }
    inline bool tyhjataulu() const { return inserts.size() == 0; }
#endif
    void postfix()
    {
        size_t a;
        /* If the table has variable-length fields, then
         * all CHAR fields longer than 3 characters are
         * automatically turned into VARCHAR fields by MySQL.
         */
        bool hasvar = false;
        for(a=0; a<fields.size(); ++a)
        {
            if(fields[a].second.gettype()=="VARCHAR"
            || fields[a].second.gettype()=="TEXT"
            || fields[a].second.gettype()=="MEDIUMTEXT"
            || fields[a].second.gettype()=="LONGTEXT"
            || fields[a].second.gettype()=="LONGBLOB"
            || fields[a].second.gettype()=="MEDIUMBLOB"
            || fields[a].second.gettype()=="BLOB")
                hasvar = true;
        }
        if(hasvar)
        {
            for(a=0; a<fields.size(); ++a)
            {
                if(fields[a].second.gettype() == "CHAR"
                && fields[a].second.getwidth() > 3)
                    fields[a].second.settype("VARCHAR");
            }
        }
        /* If the table has a collate or charset set, force
         * it into every column where it is not specified */
        for(a=0; a<fields.size(); ++a)
        {
            if(fields[a].second.getcollate().empty() && !collate.empty())
                fields[a].second.setcollate(collate);
            if(fields[a].second.getcharset().empty() && !charset.empty())
                fields[a].second.setcharset(charset);
        }
    }
};

typedef pair<string, Table> tabletype;
typedef map<string, Table> tablelisttype;

class sqldump
{
    tablelisttype tables;
    
    void load(const string &s) // analyzes the result
    {
        string Constraint;
        
        size_t a, b=s.size();
        for(a=0; a<b; ++a)
        {
            if(s[a]=='#')
            {
                while(a+1<b && s[++a]!='\n') {}
                continue;
            }
            if((a+1) < b && s[a]=='-' && s[a+1]=='-')
            {
                while(a < b && s[a] != '\n')++a;
                continue;
            }
            
            if(isempty(s[a]))continue;
            //fprintf(stderr, "*** Testing: '%s'\n", s.substr(a,50).c_str());
            if(startwith("CREATE ", s, a))
            {
                a += 7;
                SkipBlank(s, a);
                if(startwith("INDEX ", s, a))
                {
                    GetWord(s,a); // Eat "INDEX"
                    SkipBlank(s,a);
                    std::string indexname = GetWord(s,a); // Eat key name
                    SkipBlank(s,a);
                    std::string onword = GetUcaseWord(s,a);
                    if(onword != "ON")
                    {
                        throw("CREATE INDEX "+indexname+"... '"+onword+"'? I expected 'ON'");
                    }
                    SkipBlank(s,a);
                    std::string tablename = GetWord(s,a);
                    SkipBlank(s,a);

                    string content;
                    int sku=0;
                    while(a < b)
                    {
                        SkipBlank(s, a);
                        if(a >= b) break;
                        
                        size_t c = a;
                        content += QuoteWord(GetWord(s, a));
                        if(a != c) continue;
                        content += s[a];
                        
                        if(s[a] == '(')++sku;
                        else if(s[a] == ')')
                        {
                            if(!--sku)break;
                        }
                        ++a;
                    }
                    ++a; // skip ')';
                    tablelisttype::iterator
                        i = tables.find(tablename);
                    if(i == tables.end())
                    {
                        throw "No table '"+tablename+"' to add key '"+indexname+"' into";
                    }
                    i->second.addkey(indexname, content);
                    goto SkipCmd;
                }
                if(!startwith("TABLE ", s, a))goto SkipCmd;
                a += 6;
                Table t(GetWord(s, a));
                SkipBlank(s, a);
                if(a < b)
                {
                    if(s[a] == '(') ++a;
                    else
                        throw "Parse error: TABLE "+t.name()+" was not followed by '('";
                }
                while(a < b)
                {
                    SkipBlank(s, a);
                    if(s[a]=='#')
                    {
                        while(a+1<b && s[++a]!='\n') {}
                        continue;
                    }
                    string fldname = GetWord(s, a);
                    
                    if(fldname.empty())
                    {
                        if(a < b && s[a] == ')')
                        {
                            throw "Error: Table "+t.name()+" ends with comma.";
                        }
                        throw "Parse error: Was not able to get a field name in "+t.name()+".";
                    }
                    
                    SkipBlank(s, a);
                    if(ucase(fldname) == "PRIMARY")
                    {
                        if(GetUcaseWord(s, a) != "KEY")
                        {
                            throw string("Primary what?");
                        }
                        
                        string content;
                        SkipBlank(s, a);
                        
                        string indextype;
                        if(GetUcaseWord(s, a) == "USING")
                        {
                            SkipBlank(s, a);
                            indextype = GetUcaseWord(s, a);
                            SkipBlank(s, a);
                        }
                        
                        if(s[a]=='(')++a;
                        while(a < b)
                        {
                            SkipBlank(s, a);
                            size_t c=a;
                            content += QuoteWord(GetWord(s, a));
                            if(a == c)
                            {
                                throw string("Invalid parameters for PRIMARY KEY (got so far: " + content + ")");
                            }
                            if(s[a]=='(')
                            {
                                while(a < b && s[a]!=')') content += s[a++];
                                if(a < b && s[a]==')') { content += s[a++]; }
                            }
                            if(a<b && s[a]==',') { content += s[a++]; continue; }
                            break;
                        }
                        if(a >= b || s[a] != ')')
                        {
                            throw string("Expected ')' after PRIMARY KEY("+content+")");
                        }
                        ++a;
                        t.setprimary(content, indextype);
                    }
                    else if(ucase(fldname) == "KEY")
                    {
                        string indexname = GetWord(s, a);
                        string content;
                        int sku=0;
                        while(a < b)
                        {
                            SkipBlank(s, a);
                            if(a >= b) break;
                            
                            size_t c = a;
                            content += QuoteWord(GetWord(s, a));
                            if(a != c) continue;
                            content += s[a];
                            
                            if(s[a] == '(')++sku;
                            else if(s[a] == ')')
                            {
                                if(!--sku)break;
                            }
                            ++a;
                        }
                        ++a;
                        t.addkey(indexname, content);
                    }
                    else if(ucase(fldname) == "UNIQUE"
                         || ucase(fldname) == "FULLTEXT"
                         || ucase(fldname) == "SPATIAL")
                    {
                    RetryUnique:
                        string k = ucase(fldname) == "UNIQUE"
                                 ? "#"
                                 : ucase(fldname) == "SPATIAL"
                                 ? "&"
                                 : "*";
                        k += GetWord(s, a);
                        if(ucase(k.substr(1)) == "KEY")
                        {
                            SkipBlank(s, a);
                            goto RetryUnique;
                        }
                        
                        int sku=0;
                        string content;
                        while(a < b)
                        {
                            SkipBlank(s, a);
                            if(a >= b) break;
                            
                            size_t c = a;
                            content += QuoteWord(GetWord(s, a));
                            if(a != c) continue;
                            content += s[a];
                            
                            if(s[a] == '(')++sku;
                            if(s[a] == ')')
                            {
                                if(!--sku)break;
                            }
                            ++a;
                        }
                        if(a < b) ++a;
                        t.addkey(k, content);
                    }
                    else if(ucase(fldname) == "CONSTRAINT")
                    {
                        /* remember until the next FK definition */
                        SkipBlank(s, a);
                        Constraint = GetWord(s, a);
                    }
                    else if(ucase(fldname) == "FOREIGN")
                    {
                        ForeignKeyData fk;
                        fk.ConstraintName = Constraint;

                        SkipBlank(s, a);
                        if(GetUcaseWord(s, a) != "KEY")
                        {
                            throw string("Foreign what?");
                        }

                        SkipBlank(s, a);
                        if(a<b && s[a]=='(')
                            ++a;
                        else
                            throw string("Expected '(' after FOREIGN KEY");
                        while(a < b && s[a] != ')')
                        {
                            if(s[a] == ',') { fk.IndexFields += s[a++]; SkipBlank(s, a);; continue; }
                            string part = GetWord(s, a);
                            if(part.empty())
                            {
                                throw string("Couldn't get FK part (got so far: "+fk.IndexFields+")");
                            }
                            fk.IndexFields += QuoteWord(part);
                            SkipBlank(s, a);
                        }
                        if(a < b && s[a] == ')')
                            ++a;
                        else
                            throw string("Expected ')' after FOREIGN KEY("+fk.IndexFields+")");
                        
                        SkipBlank(s, a);
                        if(GetUcaseWord(s, a) != "REFERENCES")
                        {
                            throw string("Expected: 'REFERENCES'");
                        }
                        
                        SkipBlank(s, a);
                        fk.ReferenceTable = GetWord(s, a);
                        SkipBlank(s, a);
                        if(s[a]=='(')
                            ++a;
                        else
                            throw string("Expected '(' after REFERENCES");
                        while(a < b && s[a] != ')')
                        {
                            if(s[a] == ',') { fk.ReferenceFields += s[a++]; SkipBlank(s, a);; continue; }
                            string part = GetWord(s, a);
                            if(part.empty())
                            {
                                throw string("Couldn't get FK REF part ("+fk.IndexFields+")ref=("+fk.ReferenceFields+")");
                            }
                            fk.ReferenceFields += QuoteWord(part);
                            SkipBlank(s, a);
                        }
                        if(a < b && s[a] == ')')
                            ++a;
                        else
                            throw string("Expected ')' after REFERENCES("+fk.ReferenceFields+")");
                        
                        for(;;)
                        {
                            SkipBlank(s, a);
                            if(startwith("ON DELETE", s, a))
                            {
                                a += 2+1+6; SkipBlank(s, a);
                                string part = GetUcaseWord(s, a);
                                if(part == "NO" || part == "SET")
                                {
                                    SkipBlank(s, a);
                                    part += " " + GetWord(s, a);
                                }
                                fk.OnDelete = part;
                                
                                if(fk.OnDelete == "RESTRICT") fk.OnDelete = "";                                
                                continue;
                            }
                            if(startwith("ON UPDATE", s, a))
                            {
                                a += 2+1+6; SkipBlank(s, a);
                                string part = GetUcaseWord(s, a);
                                if(part == "NO" || part == "SET")
                                {
                                    SkipBlank(s, a);
                                    part += " " + GetWord(s, a);
                                }
                                fk.OnUpdate = part;
                                continue;
                            }
                            break;
                        }
                        
                        if(fk.ConstraintName.empty())
                        {
                            static size_t autonum = 1;
                            stringstream tmp;
                            tmp << "autogenerated_constraint_";
                            tmp << autonum++;
                            fk.ConstraintName = tmp.str();
                        }
                        
                        t.addforeign(fk.IndexFields, fk);
                        
                        Constraint = "";
                    }
                    else 
                    {
                        // Use a very simplistic parser to eat the parameters of this variable
                        char quotechar = 0;
                        bool empti = true;
                        int depth = 0;
                        string content;
                        while(a < b && (depth || (s[a] != ',' && s[a] != ')') || quotechar))
                        {
                            if(s[a]=='#' && !quotechar)
                            {
                                while(a+1<b && s[++a]!='\n') {}
                                continue;
                            }
                            if(isempty(s[a]))
                                empti = true;
                            else
                            {
                                if(empti && !content.empty())content += ' ';
                                content += quotechar ? s[a] : (char)toupper(s[a]);
                                empti = false;
                            }
                            if(s[a] == '(')++depth;
                            else if(s[a] == ')')--depth;
                            if(s[a] == '\\')content += s[++a];
                            else if(quotechar && s[a] == quotechar) quotechar=0;
                            else if(!quotechar && s[a] == '\'')  quotechar=s[a];
                            else if(!quotechar && s[a] == '"')  quotechar=s[a];
                            ++a;
                        }
                        if(content.find("PRIMARY KEY") != content.npos)
                        {
                            t.setprimary(QuoteWord(fldname), "");
                        }
                        if(content.empty())
                        {
                            throw string("Parse error: Got empty type for field '" + fldname + "' in "+t.name());
                        }
                        t.add(fldname, content);
                    }
                rechar_table:
                    SkipBlank(s, a);
                    if(s[a] == ',') { ++a; continue; }
                    if(s[a] == ')')break;
                    if(s[a] == '#')
                    {
                        while(a+1<b && s[++a]!='\n') {}
                        goto rechar_table;
                    }
                    
                    // We don't hold a comma necessary so that CONSTRAINT will work.
                    if(!ischar(s[a]))
                    {
                        std::string tmp; tmp += s[a];
                        throw string("Parse error: Unrecognized character '" + tmp + "' in table "+t.name()
                                    +": "+s.substr(a,10)+"...");
                    }
                }
                while(a<b)
                {
                    SkipBlank(s, a);
                    std::string word = GetUcaseWord(s, a);
                    if(word == "CHARSET")
                    {
                    GotCharSet:
                        SkipBlank(s,a);
                        if(a<b&&s[a] == '=')
                        {
                            ++a;
                            std::string type = GetUcaseWord(s,a);
                            t.setcharset(type);
                        }
                        continue;
                    }
                    if(word == "CHARACTER")
                    {
                        SkipBlank(s,a);
                        word = GetUcaseWord(s,a);
                        if(word == "SET") goto GotCharSet;
                    }
                    if(word == "COLLATE")
                    {
                        SkipBlank(s,a);
                        if(a<b&&s[a] == '=')
                        {
                            ++a;
                            std::string type = GetUcaseWord(s,a);
                            t.setcollate(type);
                        }
                        continue;
                    }
                    if(word == "TYPE")
                    {
                        SkipBlank(s,a);
                        if(a<b&&s[a] == '=')
                        {
                            ++a;
                            std::string type = GetUcaseWord(s,a);
                            t.settype(type);
                        }
                        continue;
                    }
                    if(s[a] == ';') break;
                    ++a;
                }
                t.postfix();
                tables.insert(tabletype(t.name(), t));
                
                continue;
            }
#ifdef SUPPORT_INSERTS
            if(startwith("INSERT INTO ", s, a))
            {
                a += 11;
                SkipBlank(s, a);;
                string tabname = GetWord(s, a);
                SkipBlank(s, a);;
                bool quotechar=false;
                string content;
                while(a < b && (s[a] != ';' || quotechar))
                {
                    content += s[a];
                    if(s[a] == '\\')content += s[++a];
                    else if(s[a] == '\'')quotechar = !quotechar;
                    ++a;
                }
                if(!nodata)
                    tables.find(tabname)->second.Insert(content);
            }
#endif
SkipCmd:    bool quotechar=false;
            while(a < b && (s[a] != ';' || quotechar))
            {
                if(s[a] == '\\')++a;
                else if(s[a] == '\'')quotechar = !quotechar;
                ++a;
            }
        }
    }
public:
    sqldump(FILE *fp, bool ispipe=false)
        : tables()
    {
        if(!fp)
        {
            throw InputFailedError(ispipe);
        }
        fprintf(stderr, "*** Reading%s...\n", ispipe?" pipe":" file");
        char Buf[4096];
        string s;
        for(;;)
        {
            size_t n = fread(Buf, 1, sizeof Buf, fp);
            if(n <= 0)break;
            s += string(Buf, n);
        }
        if(ispipe)
            pclose(fp);
        else
            fclose(fp);
        fprintf(stderr, "*** Analyzing...\n");
        load(s);
    }
    
    
#if 1
    class alter_table_data
    {
    public:
        std::vector<std::string/*field name*/> DROP_FIELDS;
        std::vector<std::string/*field name*/> DROP_DEFAULTS;
        std::vector<std::string/*index name*/> DROP_INDEXES;
        std::vector<std::string/*fk name*/> DROP_FOREIGN_KEYS;
        bool DROP_PRIMARY_KEY;

        std::vector<std::pair<std::string/*field name*/, std::string/*field def*/> > CHANGE_FIELDS;
        std::vector<std::pair<std::string/*field name*/, std::string/*field def*/> > ADD_FIELDS;
        std::vector<std::pair<std::string/*index name*/, std::string/*index def*/> > ADD_INDEXES;
        std::vector<std::string/*fk content*/> ADD_FOREIGN_KEYS;
        std::pair<std::string, std::string> ADD_PRIMARY;
    public:
        alter_table_data()
            : DROP_FIELDS(), DROP_DEFAULTS(),
              DROP_INDEXES(), DROP_FOREIGN_KEYS(),
              DROP_PRIMARY_KEY(), CHANGE_FIELDS(),
              ADD_FIELDS(), ADD_INDEXES(),
              ADD_FOREIGN_KEYS(), ADD_PRIMARY()
        {
        }
    };
#endif
    class alter_db_data
    {
    public:
#if 1
        std::vector<std::string/*table name*/> DROP_TABLES;
        std::vector<std::string/*table content*/> CREATE_TABLES;
        std::map<std::string/*table name*/, alter_table_data> ALTER_TABLES;
#endif
    public:
        alter_db_data()
            : DROP_TABLES(), CREATE_TABLES(), ALTER_TABLES()
        {
        }
        
        void Flush()
        {
            typedef std::vector<std::string>::const_iterator seti;
            typedef std::map<std::string, std::string>::const_iterator mapi;
            typedef std::vector<std::pair<std::string, std::string> >::const_iterator veci;
            
            /* First drop foreign keys to prevent innodb from complaining
             * when tables/keys are dropped */
            std::map<std::string, alter_table_data>::const_iterator i;
            for(i=ALTER_TABLES.begin(); i!=ALTER_TABLES.end(); ++i)
            {
                const std::string& tablename = i->first;
                const alter_table_data& data = i->second;

                std::vector<std::string> alter_list;

                for(seti j=data.DROP_FOREIGN_KEYS.begin(); j!=data.DROP_FOREIGN_KEYS.end(); ++j)
                    alter_list.push_back("DROP FOREIGN KEY " + *j);
                FlushAlterList(tablename, alter_list);
            }

            for(size_t a=0; a<DROP_TABLES.size(); ++a)
                printf("DROP TABLE %s;\n", QuoteWord(DROP_TABLES[a]).c_str());
            
            for(size_t a=0; a<CREATE_TABLES.size(); ++a)
                printf("CREATE TABLE %s;\n", CREATE_TABLES[a].c_str());
            
            for(i=ALTER_TABLES.begin(); i!=ALTER_TABLES.end(); ++i)
            {
                const std::string& tablename = i->first;
                const alter_table_data& data = i->second;
                
                std::vector<std::string> alter_list;
                
                // Drop fields.
                for(seti j=data.DROP_FIELDS.begin(); j!=data.DROP_FIELDS.end(); ++j)
                    alter_list.push_back("DROP " + QuoteWord(*j));
                for(seti j=data.DROP_DEFAULTS.begin(); j!=data.DROP_DEFAULTS.end(); ++j)
                    alter_list.push_back("ALTER " + QuoteWord(*j) + " DROP DEFAULT");
                for(seti j=data.DROP_INDEXES.begin(); j!=data.DROP_INDEXES.end(); ++j)
                {
                    if(UseCreateIndex)
                    {
                        printf("DROP INDEX %s ON %s;\n",
                            j->c_str(),
                            tablename.c_str());
                    }
                    else
                    {
                        alter_list.push_back("DROP INDEX " + *j);
                    }
                }
                if(data.DROP_PRIMARY_KEY)
                    alter_list.push_back("DROP PRIMARY KEY");
                
                FlushAlterList(tablename, alter_list);
                
                // Add fields.
                for(veci j=data.ADD_FIELDS.begin(); j!=data.ADD_FIELDS.end(); ++j)
                    alter_list.push_back("ADD " + QuoteWord(j->first) + " " + j->second);
                
                if(!data.ADD_FIELDS.empty() && !data.CHANGE_FIELDS.empty())
                {
                    // Flush here so that field orders are proper
                    FlushAlterList(tablename, alter_list);
                }
                
                // Last change fields.
                // It's done last so that changing the field order will work properly.
                for(veci j=data.CHANGE_FIELDS.begin(); j!=data.CHANGE_FIELDS.end(); ++j)
                    alter_list.push_back("CHANGE " + QuoteWord(j->first) + " " + j->second);
                    
                FlushAlterList(tablename, alter_list);
                
                for(veci j=data.ADD_INDEXES.begin(); j!=data.ADD_INDEXES.end(); ++j)
                {
                    const char *mkey = j->first.c_str();
                    if(UseCreateIndex)
                    {
                        printf("CREATE %sINDEX %s ON %s%s;\n",
                            (mkey[0]=='#' ? "UNIQUE "
                            :mkey[0]=='&' ? "SPATIAL "
                            :mkey[0]=='*' ? "FULLTEXT "
                            : ""),
                             QuoteWord((mkey[0]=='#' || mkey[0]=='&' || mkey[0]=='*') ? mkey+1 : mkey).c_str(),
                             QuoteWord(tablename).c_str(),
                             j->second.c_str());
                    }
                    else
                    {
                        alter_list.push_back("ADD " + std::string
                            (mkey[0]=='#' ? "UNIQUE"
                            :mkey[0]=='&' ? "SPATIAL"
                            :mkey[0]=='*' ? "FULLTEXT"
                            : "KEY")
                            + " "
                            + QuoteWord((mkey[0]=='#' || mkey[0]=='&' || mkey[0]=='*') ? mkey+1 : mkey)
                            + j->second);
                    }
                }
                
                if(!data.ADD_PRIMARY.first.empty())
                {
                    std::string cmd = "ADD PRIMARY KEY";
                    if(!data.ADD_PRIMARY.second.empty())
                        cmd += " USING " + data.ADD_PRIMARY.second;
                    alter_list.push_back(cmd + "(" + data.ADD_PRIMARY.first + ")");
                }

                FlushAlterList(tablename, alter_list);
            }
            
            /* Last add foreign keys. It must be done only after the keys are there. */
            for(i=ALTER_TABLES.begin(); i!=ALTER_TABLES.end(); ++i)
            {
                const std::string& tablename = i->first;
                const alter_table_data& data = i->second;
                
                std::vector<std::string> alter_list;
                
                for(seti j=data.ADD_FOREIGN_KEYS.begin(); j!=data.ADD_FOREIGN_KEYS.end(); ++j)
                {
                    alter_list.push_back("ADD FOREIGN KEY" + *j);
                }

                FlushAlterList(tablename, alter_list);
            }
        }
    private:
        void FlushAlterList(const std::string& tablename,
                            std::vector<std::string>& alter_list)
        {
            if(!alter_list.empty())
            {
                //printf("-- Table %s --\n", tablename.c_str());
                
                printf("ALTER TABLE %s ", tablename.c_str());
                for(size_t a=0; a<alter_list.size(); ++a)
                {
                    if(a>0)printf(", ");
                    printf("%s", alter_list[a].c_str());
                }
                printf(";\n");
                alter_list.clear();
            }
        }
    };
    
    void updatewith(const sqldump &newdesc)
    {
        fprintf(stderr, "*** Combining...\n");
        
        if(tables.empty())
            printf("CREATE DATABASE /*!32312 IF NOT EXISTS*/ %s;\n", QuoteWord(database).c_str());
        
        printf("USE %s;\n", QuoteWord(database).c_str());
        
        alter_db_data changes;
        
        /* Find things that no longer apply */
        for(tablelisttype::const_iterator i=tables.begin(); i!=tables.end(); ++i)
        {
            const Table& oldtable = i->second;
            
            const std::string& tablename = i->first;
            
            tablelisttype::const_iterator j = newdesc.tables.find(tablename);
            if(j == newdesc.tables.end())
            {
                /* If the table no longer exists */
                changes.DROP_TABLES.push_back(tablename);
                continue;
            }
            
            const Table& newtable = j->second;
            
            changes.ALTER_TABLES[tablename].DROP_PRIMARY_KEY=false;
            
            // Find out which fields to drop
            fieldlisttype::const_iterator k;
            for(k = oldtable.fields.begin(); k != oldtable.fields.end(); ++k)
            {
                const string& fieldname = k->first;
                if(!newtable.hasfield(fieldname))
                    changes.ALTER_TABLES[tablename].DROP_FIELDS.push_back(fieldname);
            }
            
            /* Drop extinct indices */
            keylisttype::const_iterator m;
            for(m = oldtable.keys.begin(); m != oldtable.keys.end(); ++m)
            {
                const string& keyname = m->first;
                const string oldkey   = newtable.getkey(keyname);
                if(oldkey.empty())
                {
                    const std::string Fixedname = (keyname[0]=='#' || keyname[0]=='&' || keyname[0]=='*') ? keyname.substr(1) : keyname;
                    std::string def = QuoteWord(Fixedname);
                    if(AddComments)
                        def += "\n -- Index was: " + m->second + "\n";
                    changes.ALTER_TABLES[tablename].DROP_INDEXES.push_back(def);
                }
            }
            
            /* Drop extinct foreign keys */
            foreignkeylisttype::const_iterator f;
            for(f = oldtable.fks.begin(); f != oldtable.fks.end(); ++f)
            {
                const string& keyname       = f->first;
                const ForeignKeyData oldkey = newtable.getfk(keyname);
                if(oldkey.IndexFields.empty())
                {
                    std::string def = QuoteWord(f->second.ConstraintName);
                    if(def.empty()) def = keyname;
                    if(AddComments)
                        def += "\n -- FK was: " + f->second.MakeString() + "\n";
                    changes.ALTER_TABLES[tablename].DROP_FOREIGN_KEYS.push_back(def);
                }
            }
        }
        
        /* Find all things that apply now */
        for(tablelisttype::const_iterator i=newdesc.tables.begin(); i!=newdesc.tables.end(); ++i)
        {
            const Table& newtable = i->second;
            
            bool create_table;
            std::string create_table_data;
            
            const std::string& tablename = i->first;
            
            tablelisttype::const_iterator j = tables.find(tablename);
            
            create_table = (j == tables.end());
            
            if(create_table)
            {
                /* If the table didn't exist before */
            }
            else
            {
                const Table& oldtable = j->second;
            
                if(oldtable.pkey() != newtable.pkey()
                || oldtable.pkeytype() != newtable.pkeytype())
                {
                    /*Drop the primary key if it has changed */
                    changes.ALTER_TABLES[tablename].DROP_PRIMARY_KEY = true;
                }
            }
            
            bool primarydone = false;
            bool eka = true;
            string ed = "";
            
            /* Simulation for creating proper AFTER/FIRST defs for field reordering */
            SimulateFieldOrder simulation;
            
            if(!create_table)
            {
                const Table& oldtable = j->second;

                // Add all old fields that weren't deleted
                fieldlisttype::const_iterator k;
                for(k = oldtable.fields.begin(); k != oldtable.fields.end(); ++k)
                    if(newtable.hasfield(k->first))
                        simulation.Add(k->first);
                // Next, add all new fields
                std::string prev="";
                for(k = newtable.fields.begin(); k != newtable.fields.end(); prev = (k++)->first)
                    if(!oldtable.hasfield(k->first))
                        simulation.Add_After(prev, k->first);
                // The simulation is ready.
            }
            
            /* Add new fields (drop & change already done) */
            fieldlisttype::const_iterator k;
            for(k = newtable.fields.begin(); k != newtable.fields.end(); ++k)
            {
                if(create_table)
                {
                    if(eka)eka=false;else create_table_data += ",\n";
                    
                    create_table_data += "  " + QuoteWord(k->first) + " " + k->second.c_str();
                    
                    const string defval = k->second.defval();
                    if(!defval.empty())
                        create_table_data += " DEFAULT " + defval;
                    
                    if(k->first == newtable.pkey()
                    && newtable.pkeytype().empty())
                    {
                        create_table_data += " PRIMARY KEY";
                        primarydone = true;
                    }
                }
                else
                {
                    const std::string& fieldname = k->first;
                    const Table& oldtable = j->second;
            
                    if(!oldtable.hasfield(fieldname))
                    {
                        // Add the field
                        
                        std::string def = k->second.c_str();
                        const string defval = k->second.defval();
                        if(!defval.empty())
                            def += " DEFAULT " + defval;

                        if(fieldname == newtable.pkey() && newtable.pkeytype().empty())
                        {
                            primarydone = true;
                            def += " PRIMARY KEY ";
                        }
                        if(!ed.empty())
                            def += " AFTER " + QuoteWord(ed);
                        else
                            def += " FIRST";

                        changes.ALTER_TABLES[tablename].ADD_FIELDS.
                            push_back(std::make_pair(fieldname, def));
                    }
                    else
                    {
                        // Change the field if required.
                        
                        /* old field */
                        const fieldpairtype &oldfp = oldtable.getfield(fieldname);
                        const fieldtype     &oldf  = oldfp.second;
                        const string olddefault = oldf.defval();
                        const std::string    oldprev = simulation.FindPrecedent(fieldname);
                        
                        /* new field */
                        const fieldpairtype &newfp = *k;
                        const fieldtype     &newf  = newfp.second;
                        const string newdefault = newf.defval();
                        const std::string   &newprev = ed;
                        
                        bool changed = false;
                        bool only_order = false;
                        
                        if(oldf != newf)
                        {
                            changed = true;
                            /* If the new field has no collate/charset and the old one
                             * has, ignore them */
                            fieldtype oldftemp = oldf;
                            oldftemp.clearcharsetandcollate();
                            if(oldftemp == newf) changed = false;

                            /* If the new field has no default and the old one has,
                             * and the new is "not null", ignore the default change */
                            //if(changed)fprintf(stderr, "%s changed #1\n", fieldname.c_str());
                        }
                        
                        if(newdefault != olddefault && !newdefault.empty())
                        {
                            changed = true;
                            //fprintf(stderr, "%s changed bcs def\n", fieldname.c_str());
                        }
                        if(oldprev != newprev)
                        {
                            if(!changed) only_order = true;
                            changed = true;
                            //fprintf(stderr, "%s changed bcs order\n", fieldname.c_str());
                            simulation.Move_After(newprev, fieldname);
                        }
                        
                        if(changed)
                        {
                            std::string def = QuoteWord(newfp.first) + " " + newf.c_str();
                            if(!newdefault.empty())
                            {
                                def += " DEFAULT " + newdefault;
                                if(newdefault != olddefault && AddComments)
                                    def += " -- Default was: " + olddefault + "\n";
                            }
                            if(!only_order && AddComments)
                                def += string("\n -- Field was: ") + oldf.c_str() + "\n";
                            if(oldprev != newprev)
                            {
                                if(!newprev.empty())
                                    def += " AFTER " + QuoteWord(newprev);
                                else
                                    def += " FIRST";
                                if(only_order && AddComments)
                                    def += string("\n -- Old precedent: '" + oldprev + "'\n");
                            }
                            
                            changes.ALTER_TABLES[tablename].CHANGE_FIELDS.
                                push_back(std::make_pair(fieldname, def));
                        }
                        
                        if(newdefault.empty() && !olddefault.empty())
                        {
                            std::string f = fieldname;
                            if(AddComments)
                                f += " -- Default was: " + olddefault + "\n";
                            changes.ALTER_TABLES[tablename].DROP_DEFAULTS.push_back(f);
                        }
                    }
                }
                ed = k->first.c_str();
            }
            
            /* Update/add indices */
            keylisttype::const_iterator m;
            for(m = newtable.keys.begin(); m != newtable.keys.end(); ++m)
            {
                const string& keyname = m->first;
                const char *mkey = keyname.c_str();
                if(create_table)
                {
                    if(eka)eka=false;else create_table_data += ",\n";
                    
                    create_table_data += std::string
                        (mkey[0]=='#' ? "UNIQUE"
                        :mkey[0]=='&' ? "SPATIAL"
                        :mkey[0]=='*' ? "FULLTEXT"
                        : "KEY")
                        + " "
                        + QuoteWord((mkey[0]=='#' || mkey[0]=='&' || mkey[0]=='*') ? mkey+1 : mkey)
                        + m->second;
                }
                else
                {
                    const Table& oldtable = j->second;
            
                    const string oldkey = oldtable.getkey(keyname);
                    if(oldkey != m->second)
                    {
                        if(!oldkey.empty())
                        {
                            const std::string Fixedname = (keyname[0]=='#' || keyname[0]=='&' || keyname[0]=='*') ? keyname.substr(1) : keyname;
                            std::string def = QuoteWord(Fixedname);
                            if(AddComments)
                                def += "\n -- Index was: " + oldkey + "\n";
                            changes.ALTER_TABLES[tablename].DROP_INDEXES.push_back(def);
                        }
                        // m->second shouldn't be empty
                        changes.ALTER_TABLES[tablename].ADD_INDEXES.push_back(std::make_pair(keyname, m->second));
                    }
                }
            }
            
            /* Update/add foreign keys */
            foreignkeylisttype::const_iterator f;
            for(f = newtable.fks.begin(); f != newtable.fks.end(); ++f)
            {
                const string& keyname = f->first;
                
                std::string FKdef = f->second.MakeString();
                if(create_table)
                {
                    if(eka)eka=false;else create_table_data += ",\n";
                    create_table_data += "FOREIGN KEY" + FKdef;
                }
                else
                {
                    const Table& oldtable = j->second;
            
                    const ForeignKeyData oldkey = oldtable.getfk(keyname);
                    if(oldkey != f->second)
                    {
                        if(!oldkey.IndexFields.empty())
                        {
                            std::string def = QuoteWord(oldkey.ConstraintName);
                            if(def.empty()) def = keyname;
                            if(AddComments)
                                def += "\n -- FK Was: " + oldkey.MakeString() + "\n";
                            changes.ALTER_TABLES[tablename].DROP_FOREIGN_KEYS.push_back(def);
                        }
                        // f->second shouldn't be empty
                        changes.ALTER_TABLES[tablename].ADD_FOREIGN_KEYS.push_back(FKdef);
                    }
                }
            }
            
            if(create_table)
            {
                if(!newtable.pkey().empty() && !primarydone)
                {
                    if(!eka) create_table_data += ",\n";
                    create_table_data += "  PRIMARY KEY";
                    if(!newtable.pkeytype().empty())
                        create_table_data += " USING "+newtable.pkeytype();
                    create_table_data += "(" + newtable.pkey() + ")";
                    primarydone = true;
                    eka = false;
                }
                if(!eka) create_table_data += "\n";
                
                std::string type_data;
                if(!newtable.type().empty())
                    type_data += "TYPE=" + newtable.type();
                changes.CREATE_TABLES.push_back(QuoteWord(tablename) + "\n(\n" + create_table_data + ")" + type_data);
            }
            else
            {
                const Table& oldtable = j->second;
            
                if(!primarydone && (oldtable.pkey() != newtable.pkey()
                                 || oldtable.pkeytype() != newtable.pkeytype()))
                {
                    changes.ALTER_TABLES[j->first].ADD_PRIMARY =
                       std::make_pair(newtable.pkey(), newtable.pkeytype());
                }
            }
            
#ifdef SUPPORT_INSERTS
            if(create_table || oldtable.tyhjataulu())
            {
                newtable.FlushInserts();
            }
#endif
        }
        changes.Flush();
    }
};

static void Usage(void)
{
    printf(
        "sqlupdate v"VERSION" - Copyright (C) Joel Yliluoma (http://iki.fi/bisqwit/)\n"
        "\n"
        "Usage:\n"
        "    sqlupdate [options] >changes.sql\n"
        "       (Creates an update script)\n"
        "\n"
        "Options:\n"
        "    -t tablefile          Describes the file containing\n"
        "                          the new sql layout. Default: tables.sql\n"
        "    -d database           Default: winnie3\n"
        "    -h host               Default: localhost\n"
        "    -u user               Default: root\n"
        "    -i                    Use CREATE INDEX instead of ALTER..ADD KEY\n"
        "    -m                    Add comments explaining the differences\n"
        "    -c                    Ignore character set differences\n"
        "    -p pass\n"
        "    -r                    Reverse operation. (new->old)\n"
#ifdef SUPPORT_INSERTS
        "    -D                    Insert new data\n"
#endif
        "\n"
        "Example:\n"
        "  ./sqlupdate -t etimgr.sql -d etimgr | mysql -uroot\n"
        "\n"
        "This program does not update a database. It only\n"
        "produces update scripts (which show the differences).\n\n");
}

int main(int argc, const char *const *argv)
{
    string newdescfile = "tables.sql";
    string host = "localhost";
    string user = "root";
    string pass = ""; 
    bool vanhaksi = false;
    
    while(--argc > 0)
    {
        const char *s = *++argv;
        if(*s=='-')
            while(*++s)
                switch(*s)
                {
                    #define stringparm(svar) \
                        if(*++s){svar = s;s=" ";}else{if(!--argc){Usage();return -1;};svar=*++argv;s=" ";}
                    
                    case 't': stringparm(newdescfile); break;
                    case 'd': stringparm(database); break;
                    case 'h': stringparm(host); break;
                    case 'u': stringparm(user); break;
                    case 'p': stringparm(pass); break;
                    case 'i': UseCreateIndex = true; break;
                    case 'm': AddComments = true; break;
                    case 'c': HandleCharsets = false; break;
#ifdef SUPPORT_INSERTS
                    case 'D':
                        nodata = false;
                        break;
#endif
                    case 'r':
                        vanhaksi = true;
                        break;
                    default:
                        Usage();
                        return -1;
                }
        else
        {
            Usage();
            return -1;
        }
    }
        
    string vanhakomento = "/usr/bin/mysqldump -e -u'" + user + "' -h'" + host + "'";
#ifdef SUPPORT_INSERTS
    if(nodata)
#endif
        vanhakomento += " -d";
    
    if(!pass.empty())
        vanhakomento += " -p'" + pass + "'";
    
    vanhakomento += " '";
    vanhakomento += database;
    vanhakomento += "'";
    
    try
    {
        sqldump vanha(popen(vanhakomento.c_str(), "r"), true);
        sqldump newdesc(fopen(newdescfile.c_str(), "r"));
    
        printf("SET FOREIGN_KEY_CHECKS=0;\n");
        if(vanhaksi)
            newdesc.updatewith(vanha);
        else
            vanha.updatewith(newdesc);
        printf("SET FOREIGN_KEY_CHECKS=1;\n");
    }
    catch(const InputFailedError &p)
    {
        fprintf(stderr, "*** %s failed!\n", p.pip ? database.c_str() : newdescfile.c_str());
    }
    catch(const string &s)
    {
        fprintf(stderr, "*** Oops.\n*** %s\n", s.c_str());
    }
        
    fprintf(stderr, "*** Done\n");
    
    return 0;
}
