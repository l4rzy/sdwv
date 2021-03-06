/*
 * This file part of sdcv - console version of Stardict program
 * http://sdcv.sourceforge.net
 * Copyright (C) 2005-2006 Evgeniy <dushistov@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdio>
#include <cstring>
#include <map>
#include <memory>

#include "utils.hpp"

#include "libwrapper.hpp"

//static const char ESC_BLUE[] = "<font color='blue'>";
static const char ESC_END[] = "</font>";
static const char ESC_END_B[] = "</b>";
static const char ESC_END_I[] = "</i>";
//static const char ESC_END_U[] = "</u>";
static const char ESC_BOLD[] = "<b>";
//static const char ESC_UNDERLINE[] = "<u>";
static const char ESC_ITALIC[] = "<i>";
static const char ESC_LIGHT_GRAY[] = "<font color='gray'>";
static const char ESC_BROWN[] = "<font color='brown'>";
static const char ESC_GREEN[] = "<font color='green'>";

static const char *TRANSCRIPTION_VISFMT = ESC_BROWN;
static const char *EXAMPLE_VISFMT = ESC_LIGHT_GRAY;
static const char *ABR_VISFMT = ESC_GREEN;

static std::string htmlredirect(const char *str, uint32_t &sec_size)
{
    std::string res;
    const char *p = str;
    const char needle[] = "bword://";
    //replace <A HREF="bword://reflection">reflection</A>
    //  to    <A HREF="?w=reflection">reflection</A>
    while (*p) {
        const char *f = strstr(p, needle);
        if (f) {
            res.append(p, f - p);
            res += "?w=";
            f += sizeof(needle) - 1;
            p = f;
        } else {
            uint32_t length = res.length();
            res += p;
            p += res.length() - length;
        }
    }
    sec_size = p - str;
    return res;
}
static std::string text2simplehtml(const char *str, uint32_t &sec_size)
{
    std::string res;
    const char *p = str;
    for (; *p; ++p) {
        switch (*p) {
        case ' ':
            if (p[1] == ' ')
                res += "&nbsp;";
            else
                res += ' ';
            break;
        case '\t':
            res += "&nbsp;&nbsp;&nbsp;&nbsp;";
            break;
        case '\n':
            res += "<br>\n";
            break;
        case '<':
            res += "&lt;";
            break;
        case '>':
            res += "&gt;";
            break;
        default:
            res += *p;
            break;
        }
    }
    sec_size = p - str;
    return res;
}

static std::string xdxf2text(const CBook_it &dictname, const char *xstr, bool colorize_output, uint32_t &sec_size)
{
    std::string res;
    const char *p = xstr;
    int tagFlag = 0;//1: kref. 2: rref.
    for (; *p; ++p) {
        if (*p != '<') {
            if (tagFlag == 1) {//kref | a href
                tagFlag = 0;

                const char *end_p = strchr(p, '<');
                if (end_p) {
                    const std::string targetRef(p, end_p - p);
                    res += targetRef;
                }
                res += "'>";
            } else if (tagFlag == 2) {//rref | img or wav
                tagFlag = 0;

                const char *end_p = strchr(p, '<');
                if (end_p) {
                    const std::string targetRef(p, end_p - p);
                    std::string file(dictname->second);
                    std::string::size_type end = file.rfind(G_DIR_SEPARATOR);
                    if (end != std::string::npos) {
                        std::string::size_type begin = file.rfind(G_DIR_SEPARATOR, end - 1);
                        if (begin == std::string::npos)
                            begin = 0;
                        file = file.substr(begin + 1, end - begin - 1);
                    }
                    if (file.length() <= 0 || file[0] == '/')
                        file = ".";

                    if (targetRef.find(".wav") != std::string::npos) {
                        // sound.
#if 0
                        res += "<a href='";
                        res += file;
                        res += "/res/";
                        res += targetRef;
                        res += "'>&#128266;</a>";//emoji of loudspeaker
#else
                        res += "&#128266";//emoji of loudspeaker
#endif
                        p = end_p - 1;
                        continue;
                    }
                    // assume image
                    res += "<img src='";
                    res += file;
                    res += "/res/";
                    res += targetRef;
                    res += "'>";
                    p = end_p - 1;
                    continue;
                }
            }
            if (colorize_output && *p == '\n') {
                res += "<br>";
            }
            res += *p;
            continue;
        }

        const char *next = strchr(p, '>');
        if (!next)
            continue;

        const std::string name(p + 1, next - p - 1);

        if (name == "k") {
            const char *begin = next;
            if ((next = strstr(begin, "</k>")) != nullptr)
                next += sizeof("</k>") - 1 - 1;
            else
                next = begin;
        }
        if (colorize_output) {
            if (name == "abr") {
                res += ABR_VISFMT;
            } else if (name == "/abr") {
                res += ESC_END;
            } else if (name == "kref") {
                    res += "<a href='?w=";
                    tagFlag = 1;//kref | a href
            } else if (name == "/kref") {
                    res += "</a>";
            } else if (name == "rref") {
                    tagFlag = 2;//rref | image, sound
            } else if (name == "/rref") {
                    //nothing here.
            } else if (name == "b") {
                res += ESC_BOLD;
            } else if (name == "/b") {
                res += ESC_END_B;
            } else if (name == "i") {
                res += ESC_ITALIC;
            } else if (name == "/i") {
                res += ESC_END_I;
            } else if (name == "tr") {
                res += TRANSCRIPTION_VISFMT;
                res += '[';
            } else if (name == "/tr") {
                res += ']';
                res += ESC_END;
            } else if (name == "ex") {
                res += EXAMPLE_VISFMT;
            } else if (name == "/ex") {
                res += ESC_END;
            } else if (!name.empty() && name[0] == 'c' && name != "co") {
                std::string::size_type pos = name.find('=');
                if (pos != std::string::npos) {
                    pos += 2;
                    std::string::size_type end_pos = name.find('\"', pos);
                    const std::string color(name, pos, end_pos - pos);
                    res += "<font color='" + color + "'>";
                } else {
                    res += ESC_GREEN;
                }
            } else if (name == "/c") {
                res += ESC_END;
            }
        } else {
            if (name == "tr") {
                res += '[';
            } else if (name == "/tr") {
                res += ']';
            }
        }

        p = next;
    }
    sec_size = p - xstr;
    return res;
}

static std::string parse_data(const CBook_it &dictname, const char *data, bool colorize_output)
{
    if (!data)
        return "";

    std::string res;
    uint32_t data_size, sec_size;
    const char *p = data;
    data_size = get_uint32(p);
    p += sizeof(uint32_t);
    while (uint32_t(p - data) < data_size) {
        sec_size = 0;
        switch (*p++) {
        case 'h': // HTML data
            if (*p) {
                //res += '\n';
                res += htmlredirect(p, sec_size);
            }
            sec_size++;
            break;
        case 'w': // WikiMedia markup data
        case 'm': // plain text, utf-8
        case 'l': // not utf-8, some other locale encoding, discouraged, need more work...
            if (*p) {
                if (colorize_output) {
                    res += text2simplehtml(p, sec_size);
                } else {
                    sec_size = res.length();
                    res += p;
                    sec_size = res.length() - sec_size;
                }
            }
            sec_size++;
            break;
        case 'g': // pango markup data
        case 'x': // xdxf
            if (*p) {
                res += xdxf2text(dictname, p, colorize_output, sec_size);
            }
            sec_size++;
            break;
        case 't': // english phonetic string
            if (*p) {
                if (colorize_output)
                    res += TRANSCRIPTION_VISFMT;
                res += '[';
                sec_size = res.length();
                res += p;
                sec_size = res.length() - sec_size;
                res += ']';
                if (colorize_output)
                    res += ESC_END;
            }
            sec_size++;
            break;
        case 'k': // KingSoft PowerWord data
        case 'y': // chinese YinBiao or japanese kana, utf-8
            if (*p) {
                sec_size = res.length();
                res += p;
                sec_size -= res.length();
            }
            sec_size++;
            break;
        case 'W': // wav file
        case 'P': // picture data
            sec_size = get_uint32(p);
            sec_size += sizeof(uint32_t);
            break;
        }
        p += sec_size;
    }

    return res;
}

TSearchResult::TSearchResult(const std::string &name, const std::string &w, const std::string &&def)
        : bookname(name)
        , word(w)
        , definition(def)
        , idname(name + ".." + w)
{
    std::string::size_type index;
    for (const char ch : {'\'', '\"', ' ', '\t'}) {
        while ((index = idname.find(ch)) != std::string::npos) {
            idname.replace(index, 1, 1, '_');
        }
    }
}

void Library::SimpleLookup(const std::string &str, TSearchResultList &res_list)
{
    int32_t ind;
    res_list.reserve(ndicts());
    for (int idict = 0; idict < ndicts(); ++idict)
        if (SimpleLookupWord(str.c_str(), ind, idict))
            res_list.push_back(
                TSearchResult(dict_name(idict),
                              poGetWord(ind, idict),
                              parse_data(bookname_to_ifo.find(dict_name(idict)), poGetWordData(ind, idict), param_.colorize)));
}

void Library::LookupWithFuzzy(const std::string &str, TSearchResultList &res_list)
{
    static const int MAXFUZZY = 10;

    char *fuzzy_res[MAXFUZZY];
    if (!Libs::LookupWithFuzzy(str.c_str(), fuzzy_res, MAXFUZZY))
        return;

    for (char **p = fuzzy_res, **end = (fuzzy_res + MAXFUZZY); p != end && *p; ++p) {
        SimpleLookup(*p, res_list);
        free(*p);
    }
}
void Library::LookupWithRule(const std::string &str, TSearchResultList &res_list)
{
    std::vector<char *> match_res((MAX_MATCH_ITEM_PER_LIB)*ndicts());

    const int nfound = Libs::LookupWithRule(str.c_str(), &match_res[0]);
    if (nfound == 0)
        return;

    for (int i = 0; i < nfound; ++i) {
        SimpleLookup(match_res[i], res_list);
        free(match_res[i]);
    }
}
void Library::LookupData(const std::string &str, TSearchResultList &res_list)
{
    std::vector<std::vector<char *>> drl(ndicts());
    if (!Libs::LookupData(str.c_str(), &drl[0]))
        return;
    for (int idict = 0; idict < ndicts(); ++idict)
        for (char *res : drl[idict]) {
            SimpleLookup(res, res_list);
            free(res);
        }
}

Library::response_out::response_out(const char *str, const Param_config &param, bool alldata) : param_(param), all_data(alldata)
{
    if (param_.json_output) {
        if (param_.listen_port > 0)
            buffer = "[";
        else
            putchar('[');
        return;
    }
    if (param_.colorize && all_data) {
        std::string headerhtml1 = "<html><head>"
                "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />"
                "<title>Star Dictionary</title>"
                "<style>\n"
                ".res_definition{\n"
                " table-layout: fixed;\n"
                " border-left: thin dashed black;\n"
                " border-right: thin dashed black;\n"
                " padding: 5px;\n"
                "}\n"
                ".res_word{\n"
                " table-layout: fixed;\n"
                " border: thin solid black;\n"
                " padding: 5px;\n"
                "}\n"
                "</style>\n"
                "<link href='html/jquery-ui.css' rel='stylesheet'>\n"
                "</head><body>"
                "<form id='qwFORM' action='/' method='GET'>"
                "<input id='qwt' type='text' name='w' class='ui-autocomplete-input' placeholder='input word' required='required' value='";
        std::string headerhtml2 = "'/>"
                "<input type='submit' value='GO'/>"
                "</form><hr/>\n"
                "<script src='html/jquery.js'></script>\n"
                "<script src='html/jquery-ui.js'></script>\n"
                "<script src='html/autohint.js'></script>\n"
                "<div id='dict_content'>";
        if (param_.listen_port > 0) {
            buffer = headerhtml1 + str + headerhtml2;
        } else {
            printf("%s%s%s",
            headerhtml1.c_str(),
            str,
            headerhtml2.c_str());
        }
    }
}

Library::response_out &Library::response_out::operator <<(const std::string &content) {
    if (param_.listen_port > 0) {
        buffer += content;
    } else {
        printf("%s", content.c_str());
    }
    return *this;
}
std::string Library::response_out::get_content() {
    std::string content;
    if (param_.listen_port > 0) {
        content = buffer;
        buffer = std::string();
        if (param_.json_output) {
            content += ']';
        } else if (param_.colorize && all_data) {
            content += "</div></body></html>";
        }
    } else {
        if (param_.json_output) {
            putchar(']');
        } else if (param_.colorize && all_data) {
            printf("</div></body></html>");
        }
    }
    return content;
}

void Library::response_out::print_search_result(TSearchResultList &res_list)
{
    bool first_result = true;
    response_out &out = *this;
    if (!param_.json_output && param_.colorize) {
        out << "<ol>";
        for (TSearchResult &res : res_list) {
            // put list-of-contents
            out << "<li><a href='#" << res.idname << "'>"
                << res.word << " : " << res.bookname
                << "</a></li>\n";
        }
        out << "</ol>";
    }

    for (const TSearchResult &res : res_list) {
        if (param_.json_output) {
            if (!first_result) {
                out << ",";
            } else {
                first_result = false;
            }
            out << "{\"dict\": \"" << json_escape_string(res.bookname)
                    << "\",\"word\":\"" << json_escape_string(res.word)
                    << "\",\"definition\":\"" << json_escape_string(res.definition)
                    << "\"}";

        } else {
            if (param_.colorize) {//HTML <DIV> output
                out << "<div id='"<< res.idname << "' class='res_word'>"
                    << res.bookname << " ("<< res.word
                    << ")</div><div class='res_definition'>"
                    << res.definition << "</div>";
            } else {
                out << "-->" << res.bookname
                    << "\n-->" << res.word
                    << "\n"<< res.definition << "\n\n";
            }
        }
    }
}

const std::string Library::process_phrase(const char *str, bool alldata)
{
    response_out outputer(str, param_, alldata);
    if (nullptr == str || '\0' == str[0])
        return outputer.get_content();

    std::string query;

    //analyze_query(str, query);

//    size_t bytes_read;
//    size_t bytes_written;
//    glib::Error err;
//    glib::CharStr str;
//    if (!utf8_input_)
//        str.reset(g_locale_to_utf8(loc_str, -1, &bytes_read, &bytes_written, get_addr(err)));
//    else
//        str.reset(g_strdup(loc_str));
//
//    if (nullptr == get_impl(str)) {
//        fprintf(stderr, _("Can not convert %s to utf8.\n"), loc_str);
//        fprintf(stderr, "%s\n", err->message);
//        return std::string();
//    }

    TSearchResultList res_list;

    switch (analyze_query(str, query)) {
    case qtFUZZY:
        LookupWithFuzzy(query, res_list);
        break;
    case qtREGEXP:
        LookupWithRule(query, res_list);
        break;
    case qtSIMPLE:
        SimpleLookup(query, res_list);
        if (res_list.empty() && !param_.no_fuzzy)
            LookupWithFuzzy(str, res_list);
        break;
    case qtDATA:
        LookupData(query, res_list);
        break;
    default:
        /*nothing*/;
    }

    if (!res_list.empty()) {
        outputer.print_search_result(res_list);
    } else {
        if (!param_.json_output) {
            outputer << ("Nothing similar to ") << (str);
        }
    }

    return outputer.get_content();
}
const std::string Library::get_neighbour(const char *str, int offset, uint32_t length)
{
    std::list<std::string> neighbour;
    if (nullptr == str || '\0' == str[0])
        return "";

    int32_t *icurr = (int32_t*)malloc(sizeof(int32_t) * ndicts());
    const char *word;

    poGetNextWord(str, icurr);
    while (++offset <= 2) {
        word = poGetPreWord(icurr);
        if (!word)
            break;
    }
    while (neighbour.size() < length) {
        word = poGetNextWord(nullptr, icurr);
        if (word)
            neighbour.push_back(word);
        else
            break;
    }
    free(icurr);

    std::string result;
    for (const auto &e : neighbour) {
        result += e + "\n";
    }
    result.resize(result.length() - 1);//delete last "\n"
    return result;
}
