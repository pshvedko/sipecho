/*
 * xml.c
 *
 *  Created on: Sep 24, 2013
 *      Author: shved
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xml.h"

static const struct xml_entity __xml_entity[] = {

    xml_entity_INITIALIZER("", 0, nil),

    xml_entity_INITIALIZER("\"", 34, quot),
    xml_entity_INITIALIZER("'", 39, apos),
    xml_entity_INITIALIZER("&", 38, amp),
    xml_entity_INITIALIZER("<", 60, lt),
    xml_entity_INITIALIZER(">", 62, gt),

    xml_entity_INITIALIZER(" ", 160, nbsp),
    xml_entity_INITIALIZER("¡", 161, iexcl),
    xml_entity_INITIALIZER("¢", 162, cent),
    xml_entity_INITIALIZER("£", 163, pound),
    xml_entity_INITIALIZER("¤", 164, curren),
    xml_entity_INITIALIZER("¥", 165, yen),
    xml_entity_INITIALIZER("¦", 166, brvbar),
    xml_entity_INITIALIZER("§", 167, sect),
    xml_entity_INITIALIZER("¨", 168, uml),
    xml_entity_INITIALIZER("©", 169, copy),
    xml_entity_INITIALIZER("ª", 170, ordf),
    xml_entity_INITIALIZER("«", 171, laquo),
    xml_entity_INITIALIZER("¬", 172, not),
    xml_entity_INITIALIZER("", 173, shy),
    xml_entity_INITIALIZER("®", 174, reg),
    xml_entity_INITIALIZER("¯", 175, macr),
    xml_entity_INITIALIZER("°", 176, deg),
    xml_entity_INITIALIZER("±", 177, plusmn),
    xml_entity_INITIALIZER("²", 178, sup2),
    xml_entity_INITIALIZER("³", 179, sup3),
    xml_entity_INITIALIZER("´", 180, acute),
    xml_entity_INITIALIZER("µ", 181, micro),
    xml_entity_INITIALIZER("¶", 182, para),
    xml_entity_INITIALIZER("·", 183, middot),
    xml_entity_INITIALIZER("¸", 184, cedil),
    xml_entity_INITIALIZER("¹", 185, sup1),
    xml_entity_INITIALIZER("º", 186, ordm),
    xml_entity_INITIALIZER("»", 187, raquo),
    xml_entity_INITIALIZER("¼", 188, frac14),
    xml_entity_INITIALIZER("½", 189, frac12),
    xml_entity_INITIALIZER("¾", 190, frac34),
    xml_entity_INITIALIZER("¿", 191, iquest),
    xml_entity_INITIALIZER("×", 215, times),
    xml_entity_INITIALIZER("÷", 247, divide),

    xml_entity_INITIALIZER("À", 192, Agrave),
    xml_entity_INITIALIZER("Á", 193, Aacute),
    xml_entity_INITIALIZER("Â", 194, Acirc),
    xml_entity_INITIALIZER("Ã", 195, Atilde),
    xml_entity_INITIALIZER("Ä", 196, Auml),
    xml_entity_INITIALIZER("Å", 197, Aring),
    xml_entity_INITIALIZER("Æ", 198, AElig),
    xml_entity_INITIALIZER("Ç", 199, Ccedil),
    xml_entity_INITIALIZER("È", 200, Egrave),
    xml_entity_INITIALIZER("É", 201, Eacute),
    xml_entity_INITIALIZER("Ê", 202, Ecirc),
    xml_entity_INITIALIZER("Ë", 203, Euml),
    xml_entity_INITIALIZER("Ì", 204, Igrave),
    xml_entity_INITIALIZER("Í", 205, Iacute),
    xml_entity_INITIALIZER("Î", 206, Icirc),
    xml_entity_INITIALIZER("Ï", 207, Iuml),
    xml_entity_INITIALIZER("Ð", 208, ETH),
    xml_entity_INITIALIZER("Ñ", 209, Ntilde),
    xml_entity_INITIALIZER("Ò", 210, Ograve),
    xml_entity_INITIALIZER("Ó", 211, Oacute),
    xml_entity_INITIALIZER("Ô", 212, Ocirc),
    xml_entity_INITIALIZER("Õ", 213, Otilde),
    xml_entity_INITIALIZER("Ö", 214, Ouml),
    xml_entity_INITIALIZER("Ø", 216, Oslash),
    xml_entity_INITIALIZER("Ù", 217, Ugrave),
    xml_entity_INITIALIZER("Ú", 218, Uacute),
    xml_entity_INITIALIZER("Û", 219, Ucirc),
    xml_entity_INITIALIZER("Ü", 220, Uuml),
    xml_entity_INITIALIZER("Ý", 221, Yacute),
    xml_entity_INITIALIZER("Þ", 222, THORN),
    xml_entity_INITIALIZER("ß", 223, szlig),
    xml_entity_INITIALIZER("à", 224, agrave),
    xml_entity_INITIALIZER("á", 225, aacute),
    xml_entity_INITIALIZER("â", 226, acirc),
    xml_entity_INITIALIZER("ã", 227, atilde),
    xml_entity_INITIALIZER("ä", 228, auml),
    xml_entity_INITIALIZER("å", 229, aring),
    xml_entity_INITIALIZER("æ", 230, aelig),
    xml_entity_INITIALIZER("ç", 231, ccedil),
    xml_entity_INITIALIZER("è", 232, egrave),
    xml_entity_INITIALIZER("é", 233, eacute),
    xml_entity_INITIALIZER("ê", 234, ecirc),
    xml_entity_INITIALIZER("ë", 235, euml),
    xml_entity_INITIALIZER("ì", 236, igrave),
    xml_entity_INITIALIZER("í", 237, iacute),
    xml_entity_INITIALIZER("î", 238, icirc),
    xml_entity_INITIALIZER("ï", 239, iuml),
    xml_entity_INITIALIZER("ð", 240, eth),
    xml_entity_INITIALIZER("ñ", 241, ntilde),
    xml_entity_INITIALIZER("ò", 242, ograve),
    xml_entity_INITIALIZER("ó", 243, oacute),
    xml_entity_INITIALIZER("ô", 244, ocirc),
    xml_entity_INITIALIZER("õ", 245, otilde),
    xml_entity_INITIALIZER("ö", 246, ouml),
    xml_entity_INITIALIZER("ø", 248, oslash),
    xml_entity_INITIALIZER("ù", 249, ugrave),
    xml_entity_INITIALIZER("ú", 250, uacute),
    xml_entity_INITIALIZER("û", 251, ucirc),
    xml_entity_INITIALIZER("ü", 252, uuml),
    xml_entity_INITIALIZER("ý", 253, yacute),
    xml_entity_INITIALIZER("þ", 254, thorn),
    xml_entity_INITIALIZER("ÿ", 255, yuml),

    xml_entity_INITIALIZER("∀", 8704, forall),
    xml_entity_INITIALIZER("∂", 8706, part),
    xml_entity_INITIALIZER("∃", 8707, exist),
    xml_entity_INITIALIZER("∅", 8709, empty),
    xml_entity_INITIALIZER("∇", 8711, nabla),
    xml_entity_INITIALIZER("∈", 8712, isin),
    xml_entity_INITIALIZER("∉", 8713, notin),
    xml_entity_INITIALIZER("∋", 8715, ni),
    xml_entity_INITIALIZER("∏", 8719, prod),
    xml_entity_INITIALIZER("∑", 8721, sum),
    xml_entity_INITIALIZER("−", 8722, minus),
    xml_entity_INITIALIZER("∗", 8727, lowast),
    xml_entity_INITIALIZER("√", 8730, radic),
    xml_entity_INITIALIZER("∝", 8733, prop),
    xml_entity_INITIALIZER("∞", 8734, infin),
    xml_entity_INITIALIZER("∠", 8736, ang),
    xml_entity_INITIALIZER("∧", 8743, and),
    xml_entity_INITIALIZER("∨", 8744, or),
    xml_entity_INITIALIZER("∩", 8745, cap),
    xml_entity_INITIALIZER("∪", 8746, cup),
    xml_entity_INITIALIZER("∫", 8747, int),
    xml_entity_INITIALIZER("∴", 8756, there4),
    xml_entity_INITIALIZER("∼", 8764, sim),
    xml_entity_INITIALIZER("≅", 8773, cong),
    xml_entity_INITIALIZER("≈", 8776, asymp),
    xml_entity_INITIALIZER("≠", 8800, ne),
    xml_entity_INITIALIZER("≡", 8801, equiv),
    xml_entity_INITIALIZER("≤", 8804, le),
    xml_entity_INITIALIZER("≥", 8805, ge),
    xml_entity_INITIALIZER("⊂", 8834, sub),
    xml_entity_INITIALIZER("⊃", 8835, sup),
    xml_entity_INITIALIZER("⊄", 8836, nsub),
    xml_entity_INITIALIZER("⊆", 8838, sube),
    xml_entity_INITIALIZER("⊇", 8839, supe),
    xml_entity_INITIALIZER("⊕", 8853, oplus),
    xml_entity_INITIALIZER("⊗", 8855, otimes),
    xml_entity_INITIALIZER("⊥", 8869, perp),
    xml_entity_INITIALIZER("⋅", 8901, sdot),

    xml_entity_INITIALIZER("Α", 913, Alpha),
    xml_entity_INITIALIZER("Β", 914, Beta),
    xml_entity_INITIALIZER("Γ", 915, Gamma),
    xml_entity_INITIALIZER("Δ", 916, Delta),
    xml_entity_INITIALIZER("Ε", 917, Epsilon),
    xml_entity_INITIALIZER("Ζ", 918, Zeta),
    xml_entity_INITIALIZER("Η", 919, Eta),
    xml_entity_INITIALIZER("Θ", 920, Theta),
    xml_entity_INITIALIZER("Ι", 921, Iota),
    xml_entity_INITIALIZER("Κ", 922, Kappa),
    xml_entity_INITIALIZER("Λ", 923, Lambda),
    xml_entity_INITIALIZER("Μ", 924, Mu),
    xml_entity_INITIALIZER("Ν", 925, Nu),
    xml_entity_INITIALIZER("Ξ", 926, Xi),
    xml_entity_INITIALIZER("Ο", 927, Omicron),
    xml_entity_INITIALIZER("Π", 928, Pi),
    xml_entity_INITIALIZER("Ρ", 929, Rho),
    xml_entity_INITIALIZER("", 930, Sigmaf),
    xml_entity_INITIALIZER("Σ", 931, Sigma),
    xml_entity_INITIALIZER("Τ", 932, Tau),
    xml_entity_INITIALIZER("Υ", 933, Upsilon),
    xml_entity_INITIALIZER("Φ", 934, Phi),
    xml_entity_INITIALIZER("Χ", 935, Chi),
    xml_entity_INITIALIZER("Ψ", 936, Psi),
    xml_entity_INITIALIZER("Ω", 937, Omega),

    xml_entity_INITIALIZER("α", 945, alpha),
    xml_entity_INITIALIZER("β", 946, beta),
    xml_entity_INITIALIZER("γ", 947, gamma),
    xml_entity_INITIALIZER("δ", 948, delta),
    xml_entity_INITIALIZER("ε", 949, epsilon),
    xml_entity_INITIALIZER("ζ", 950, zeta),
    xml_entity_INITIALIZER("η", 951, eta),
    xml_entity_INITIALIZER("θ", 952, theta),
    xml_entity_INITIALIZER("ι", 953, iota),
    xml_entity_INITIALIZER("κ", 954, kappa),
    xml_entity_INITIALIZER("λ", 955, lambda),
    xml_entity_INITIALIZER("μ", 956, mu),
    xml_entity_INITIALIZER("ν", 957, nu),
    xml_entity_INITIALIZER("ξ", 958, xi),
    xml_entity_INITIALIZER("ο", 959, omicron),
    xml_entity_INITIALIZER("π", 960, pi),
    xml_entity_INITIALIZER("ρ", 961, rho),
    xml_entity_INITIALIZER("ς", 962, sigmaf),
    xml_entity_INITIALIZER("σ", 963, sigma),
    xml_entity_INITIALIZER("τ", 964, tau),
    xml_entity_INITIALIZER("υ", 965, upsilon),
    xml_entity_INITIALIZER("φ", 966, phi),
    xml_entity_INITIALIZER("χ", 967, chi),
    xml_entity_INITIALIZER("ψ", 968, psi),
    xml_entity_INITIALIZER("ω", 969, omega),

    xml_entity_INITIALIZER("ϑ", 977, thetasym),
    xml_entity_INITIALIZER("ϒ", 978, upsih),
    xml_entity_INITIALIZER("ϖ", 982, piv),

    xml_entity_INITIALIZER("Œ", 338, OElig),
    xml_entity_INITIALIZER("œ", 339, oelig),
    xml_entity_INITIALIZER("Š", 352, Scaron),
    xml_entity_INITIALIZER("š", 353, scaron),
    xml_entity_INITIALIZER("Ÿ", 376, Yuml),
    xml_entity_INITIALIZER("ƒ", 402, fnof),
    xml_entity_INITIALIZER("ˆ", 710, circ),
    xml_entity_INITIALIZER("˜", 732, tilde),
    xml_entity_INITIALIZER(" ", 8194, ensp),
    xml_entity_INITIALIZER(" ", 8195, emsp),
    xml_entity_INITIALIZER(" ", 8201, thinsp),
    xml_entity_INITIALIZER("", 8204, zwnj),
    xml_entity_INITIALIZER("", 8205, zwj),
    xml_entity_INITIALIZER("", 8206, lrm),
    xml_entity_INITIALIZER("", 8207, rlm),

    xml_entity_INITIALIZER("–", 8211, ndash),
    xml_entity_INITIALIZER("—", 8212, mdash),
    xml_entity_INITIALIZER("‘", 8216, lsquo),
    xml_entity_INITIALIZER("’", 8217, rsquo),
    xml_entity_INITIALIZER("‚", 8218, sbquo),
    xml_entity_INITIALIZER("“", 8220, ldquo),
    xml_entity_INITIALIZER("”", 8221, rdquo),
    xml_entity_INITIALIZER("„", 8222, bdquo),
    xml_entity_INITIALIZER("†", 8224, dagger),
    xml_entity_INITIALIZER("‡", 8225, Dagger),
    xml_entity_INITIALIZER("•", 8226, bull),
    xml_entity_INITIALIZER("…", 8230, hellip),
    xml_entity_INITIALIZER("‰", 8240, permil),
    xml_entity_INITIALIZER("′", 8242, prime),
    xml_entity_INITIALIZER("″", 8243, Prime),
    xml_entity_INITIALIZER("‹", 8249, lsaquo),
    xml_entity_INITIALIZER("›", 8250, rsaquo),
    xml_entity_INITIALIZER("‾", 8254, oline),
    xml_entity_INITIALIZER("€", 8364, euro),
    xml_entity_INITIALIZER("™", 8482, trade),
    xml_entity_INITIALIZER("←", 8592, larr),
    xml_entity_INITIALIZER("↑", 8593, uarr),
    xml_entity_INITIALIZER("→", 8594, rarr),
    xml_entity_INITIALIZER("↓", 8595, darr),
    xml_entity_INITIALIZER("↔", 8596, harr),
    xml_entity_INITIALIZER("↵", 8629, crarr),
    xml_entity_INITIALIZER("⌈", 8968, lceil),
    xml_entity_INITIALIZER("⌉", 8969, rceil),
    xml_entity_INITIALIZER("⌊", 8970, lfloor),
    xml_entity_INITIALIZER("⌋", 8971, rfloor),

    xml_entity_INITIALIZER("◊", 9674, loz),
    xml_entity_INITIALIZER("♠", 9824, spades),
    xml_entity_INITIALIZER("♣", 9827, clubs),
    xml_entity_INITIALIZER("♥", 9829, hearts),
    xml_entity_INITIALIZER("♦", 9830, diams),
};

#ifndef _XML_HTML_TAG

static const struct xml_tag __xml_tag[] = {
    xml_tag_INITIALIZER(?xml, none, follow),
    xml_tag_INITIALIZER(!--, none, comment),
};

static const int __xml_map[1 + 1 + 1] = {0, 2, 2};

#else

static const struct xml_tag __xml_tag[] = {
	xml_tag_INITIALIZER( !doctype, none, follow ),
	xml_tag_INITIALIZER( !--, none, comment ),
	//2
	xml_tag_INITIALIZER( a, similar, follow ),
	xml_tag_INITIALIZER( abbr, mandatory, follow ),
	xml_tag_INITIALIZER( acronym, mandatory, follow ),
	xml_tag_INITIALIZER( address, mandatory, follow ),
	xml_tag_INITIALIZER( applet, mandatory, follow ),
	xml_tag_INITIALIZER( area, none, follow ),
	xml_tag_INITIALIZER( article, mandatory, follow ),
	xml_tag_INITIALIZER( aside, mandatory, follow ),
	xml_tag_INITIALIZER( audio, mandatory, follow ),
	//11
	xml_tag_INITIALIZER( b, mandatory, follow ),
	xml_tag_INITIALIZER( base, none, follow ),
	xml_tag_INITIALIZER( basefont, none, follow ),
	xml_tag_INITIALIZER( bdo, mandatory, follow ),
	xml_tag_INITIALIZER( bgsound, none, follow ),
	xml_tag_INITIALIZER( blockquote, mandatory, follow ),
	xml_tag_INITIALIZER( big, mandatory, follow ),
	xml_tag_INITIALIZER( body, mandatory, follow ),
	xml_tag_INITIALIZER( blink, mandatory, follow ),
	xml_tag_INITIALIZER( br, none, follow ),
	xml_tag_INITIALIZER( button, mandatory, follow ),
	//22
	xml_tag_INITIALIZER( canvas, mandatory, follow ),
	xml_tag_INITIALIZER( caption, mandatory, follow ),
	xml_tag_INITIALIZER( center, mandatory, follow ),
	xml_tag_INITIALIZER( cite, mandatory, follow ),
	xml_tag_INITIALIZER( code, mandatory, comment ),
	xml_tag_INITIALIZER( col, none, follow ),
	xml_tag_INITIALIZER( colgroup, mandatory, follow ),
	xml_tag_INITIALIZER( command, mandatory, follow ),
	xml_tag_INITIALIZER( comment, mandatory, comment ),
	//31
	xml_tag_INITIALIZER( datalist, mandatory, follow ),
	xml_tag_INITIALIZER( dd, mandatory, follow ),
	xml_tag_INITIALIZER( del, mandatory, follow ),
	xml_tag_INITIALIZER( details, mandatory, follow ),
	xml_tag_INITIALIZER( dfn, mandatory, follow ),
	xml_tag_INITIALIZER( dir, mandatory, follow ),
	xml_tag_INITIALIZER( div, mandatory, follow ),
	xml_tag_INITIALIZER( dl, mandatory, follow ),
	xml_tag_INITIALIZER( dt, mandatory, follow ),
	//40
	xml_tag_INITIALIZER( em, mandatory, follow ),
	xml_tag_INITIALIZER( embed, any, follow ),
	//42
	xml_tag_INITIALIZER( fieldset, mandatory, follow ),
	xml_tag_INITIALIZER( figcaption, mandatory, follow ),
	xml_tag_INITIALIZER( figure, mandatory, follow ),
	xml_tag_INITIALIZER( font, mandatory, follow ),
	xml_tag_INITIALIZER( form, mandatory, follow ),
	xml_tag_INITIALIZER( footer, mandatory, follow ),
	xml_tag_INITIALIZER( frame, none, follow ),
	xml_tag_INITIALIZER( frameset, mandatory, follow ),
	//50
	xml_tag_INITIALIZER( h1, mandatory, follow ),
	xml_tag_INITIALIZER( h2, mandatory, follow ),
	xml_tag_INITIALIZER( h3, mandatory, follow ),
	xml_tag_INITIALIZER( h4, mandatory, follow ),
	xml_tag_INITIALIZER( h5, mandatory, follow ),
	xml_tag_INITIALIZER( h6, mandatory, follow ),
	xml_tag_INITIALIZER( head, mandatory, follow ),
	xml_tag_INITIALIZER( header, mandatory, follow ),
	xml_tag_INITIALIZER( hgroup, mandatory, follow ),
	xml_tag_INITIALIZER( hr, none, follow ),
	xml_tag_INITIALIZER( html, mandatory, follow ),
	//61
	xml_tag_INITIALIZER( i, mandatory, follow ),
	xml_tag_INITIALIZER( iframe, mandatory, follow ),
	xml_tag_INITIALIZER( img, none, follow ),
	xml_tag_INITIALIZER( input, none, follow ),
	xml_tag_INITIALIZER( ins, mandatory, follow ),
	xml_tag_INITIALIZER( isindex, none, follow ),
	//67
	xml_tag_INITIALIZER( kbd, mandatory, follow ),
	xml_tag_INITIALIZER( keygen, none, follow ),
	//69
	xml_tag_INITIALIZER( label, mandatory, follow ),
	xml_tag_INITIALIZER( legend, mandatory, follow ),
	xml_tag_INITIALIZER( li, similar, follow ),
	xml_tag_INITIALIZER( link, none, follow ),
	//  xml_tag_INITIALIZER( listing, mandatory, comment ),
	//73
	xml_tag_INITIALIZER( map, mandatory, follow ),
	xml_tag_INITIALIZER( marquee, mandatory, follow ),
	xml_tag_INITIALIZER( mark, mandatory, follow ),
	xml_tag_INITIALIZER( menu, mandatory, follow ),
	xml_tag_INITIALIZER( meta, none, follow ),
	xml_tag_INITIALIZER( meter, mandatory, follow ),
	//  xml_tag_INITIALIZER( multicol, mandatory, follow ),
	//79
	xml_tag_INITIALIZER( nav, mandatory, follow ),
	xml_tag_INITIALIZER( nobr, mandatory, follow ),
	xml_tag_INITIALIZER( noembed, mandatory, follow ),
	xml_tag_INITIALIZER( noframes, mandatory, follow ),
	xml_tag_INITIALIZER( noindex, mandatory, follow ),
	xml_tag_INITIALIZER( noscript, mandatory, follow ),
	//85
	xml_tag_INITIALIZER( object, mandatory, follow ),
	xml_tag_INITIALIZER( ol, mandatory, follow ),
	xml_tag_INITIALIZER( optgroup, mandatory, follow ),
	xml_tag_INITIALIZER( option, any, follow ),
	xml_tag_INITIALIZER( output, mandatory, follow ),
	//90
	xml_tag_INITIALIZER( p, similar, follow ),
	xml_tag_INITIALIZER( param, none, follow ),
	xml_tag_INITIALIZER( plaintext, any, comment ),
	xml_tag_INITIALIZER( pre, mandatory, comment ),
	xml_tag_INITIALIZER( progress, mandatory, follow ),
	//95
	xml_tag_INITIALIZER( q, mandatory, follow ),
	//96
	xml_tag_INITIALIZER( rp, any, follow ),
	xml_tag_INITIALIZER( rt, any, follow ),
	xml_tag_INITIALIZER( ruby, mandatory, follow ),
	//99
	xml_tag_INITIALIZER( s, mandatory, follow ),
	xml_tag_INITIALIZER( samp, mandatory, follow ),
	xml_tag_INITIALIZER( script, mandatory, comment ),
	xml_tag_INITIALIZER( section, mandatory, follow ),
	xml_tag_INITIALIZER( select, mandatory, follow ),
	xml_tag_INITIALIZER( small, mandatory, follow ),
	//  xml_tag_INITIALIZER( spacer, any, follow ),
	xml_tag_INITIALIZER( span, mandatory, follow ),
	xml_tag_INITIALIZER( source, any, follow ),
	xml_tag_INITIALIZER( strike, mandatory, follow ),
	xml_tag_INITIALIZER( strong, mandatory, follow ),
	xml_tag_INITIALIZER( style, mandatory, comment ),
	xml_tag_INITIALIZER( sub, mandatory, follow ),
	xml_tag_INITIALIZER( summary, mandatory, follow ),
	xml_tag_INITIALIZER( sup, mandatory, follow ),
	//113
	xml_tag_INITIALIZER( table, mandatory, follow ),
	xml_tag_INITIALIZER( tbody, any, follow ),
	xml_tag_INITIALIZER( td, any, follow ),
	xml_tag_INITIALIZER( textarea, mandatory, follow ),
	xml_tag_INITIALIZER( tfoot, any, follow ),
	xml_tag_INITIALIZER( th, any, follow ),
	xml_tag_INITIALIZER( thead, any, follow ),
	xml_tag_INITIALIZER( time, mandatory, follow ),
	xml_tag_INITIALIZER( title, mandatory, follow ),
	xml_tag_INITIALIZER( tr, any, follow ),
	xml_tag_INITIALIZER( tt, mandatory, follow ),
	//124
	xml_tag_INITIALIZER( u, mandatory, follow ),
	xml_tag_INITIALIZER( ul, mandatory, follow ),
	//126
	xml_tag_INITIALIZER( var, mandatory, follow ),
	xml_tag_INITIALIZER( video, mandatory, follow ),
	//128
	xml_tag_INITIALIZER( wbr, none, follow ),
	//129
	xml_tag_INITIALIZER( xmp, mandatory, follow ),
};

static const int __xml_map[1 + 'z' - 'a' + 1 + 1] = {
	0,
	2,
	11,
	22,
	31,
	40,
	42,
	50,
	50,
	61,
	67,
	67,
	69,
	73,
	79,
	85,
	90,
	95,
	96,
	99,
	113,
	124,
	126,
	128,
	129,
	130,
	130,
	130};

#endif

static const struct xml_tag __xml_unknown_tag[] = {

    xml_tag_INITIALIZER(__, any, follow),
};

/**
 *
 */
int xml_is_comment(const char *end, const int comment) {
    if (comment)
        if (memcmp(end - 2, "--", 2) != 0)
            return 0;
    return 1;
}

/**
 *
 */
struct xml_text xml_text_set(const char *v, const long unsigned l) {
    const struct xml_text text = {.value = v, .length = l, .needfree = 0};
    return text;
}

/**
 *
 */
unsigned xml_text_cmp(const struct xml_text *a, const struct xml_text *b) {
    if (a->length == b->length)
        for (int i = 0; i < a->length; i++)
            if (tolower(a->value[i]) != tolower(b->value[i]))
                return a->value[i] - b->value[i];
    return a->length - b->length;
}

/**
 *
 */
const struct xml_tag *xml_tag_find(struct xml_text *name) {
    if (name->length) {
        int m = 1 + tolower(*name->value) - 'a';
        if (m < 0)
            m = 0;
        else if (m >= sizeof(__xml_map) / sizeof(*__xml_map))
            m = sizeof(__xml_map) / sizeof(*__xml_map) - 2;

        int i = __xml_map[m];

        for (; i < __xml_map[m + 1]; i++)
            if (!xml_text_cmp(name, &__xml_tag[i].name))
                return __xml_tag + i;
    }
    return __xml_unknown_tag;
}

/**
 *
 */
int xml_is_closing(const char *begin, struct xml_element *node) {
    if (node)
        if (*++begin != '/' || memcmp(++begin, node->name.value, node->name.length) != 0)
            return 0;
    return 1;
}

/**
 *
 */
size_t xml_next(struct xml_element *node, const char *begin, const char *end) {
    const char *from = begin;
    const char *to = end;

    node->type = xml_empty_node;

    if (begin != end) {
        while (from != end)
            if (*from == '<' && xml_is_closing(from, node->parent))
                break;
            else
                from++;

        if (from != begin) {
            to = from;
            node->type = xml_text_node;
            node->content.value = begin;
            node->content.length = to - begin;
        } else if (from != end) {
            int comment = 0;
            to = from + 1;

            if (!memcmp(to, "!--", 3)) {
                comment = 1;
                to += 3;
            } else {
                if (*to == '/') {
                    to++;
                    node->shape = xml_shape_close;
                }
                node->name.value = to;

                while (*to == '!' || *to == '?' || isalnum(*to) || *to == '_' || *to == ':' || *to == '-')
                    to++;

                if (to == node->name.value) {
                    node->type = xml_empty_node;
                    return to - begin;
                }
                node->name.length = to - node->name.value;
                node->tag = xml_tag_find(&node->name);
                node->content.value = to;
            }

            while (to != end)
                if (!comment && *to == '<') {
                    node->type = xml_empty_node;
                    return to - begin;
                } else if (*to == '>' && xml_is_comment(to, comment))
                    break;
                else
                    to++;

            if (to != end) {
                if (!comment) {
                    node->type = xml_element_node;
                    node->content.length = to - node->content.value;

                    if (to[-1] == '/' && node->shape == xml_shape_open) {
                        node->shape = xml_shape_single;
                        node->content.length--;
                    }
                }
                to++;
            }
        }
    }
    return to - begin;
}

/**
 *
 */
struct xml_element *xml_element_add(struct xml_element *root, struct xml_element *node) {
    switch (node->type) {
        case xml_element_node:
        case xml_text_node:
            if (root->last) {
                node->id = root->last->id + 1;
                root->last = root->last->next = node;
            } else
                root->last = root->children = node;
            break;

        case xml_attribute_node:
            node->next = root->attribute;
            root->attribute = node;
            break;

        case xml_empty_node:
        case xml_document_node:
            // XXX
            break;
    }
    node->parent = root;
    node->level = root->level + 1;

    return node;
}

/**
 *
 */
struct xml_element *xml_element_append(struct xml_element *root, struct xml_element *node) {
    if (root->level > xml_max_DEEP)
        return 0;

    struct xml_element *add = malloc(sizeof(struct xml_element));
    if (add) {
        node->parent = root;
        node->level = root->level + 1;

        if (root->last) {
            node->id = root->last->id + 1;
            root->last = root->last->next = add;
        } else
            root->last = root->children = add;

        memcpy(add, node, sizeof(struct xml_element));
    }
    return add;
}

/**
 *
 */
int xml_text_is_empty(struct xml_text *text) {
    for (int i = 0; i < text->length; i++)
        if (!isspace(text->value[i]))
            return 0;
    return 1;
}

/**
 *
 */
int xml_feed(struct xml *xml, const char *buff, const size_t len) {
    if (!xml)
        return 0;

    size_t begin = 0;
    struct xml_element *root = &xml->document;
    struct xml_element *comment = NULL;

    while (begin < len) {
        struct xml_element node = xml_element_INITIALIZER;
        struct xml_element *element = NULL;

        node.parent = comment;
        begin += xml_next(&node, buff + begin, buff + len);
        comment = NULL;

        switch (node.type) {
            case xml_attribute_node:
            case xml_document_node:
                break;

            case xml_element_node:
                switch (node.shape) {
                    case xml_shape_open:
                    case xml_shape_single:
                        if (!root)
                            return -1;

                        if (node.tag->brace == xml_brace_similar && node.tag == root->tag)
                            root = root->parent;

                        element = xml_element_append(root, &node);
                        if (!element)
                            return -1;

                        xml_node_compile(element);

                        if (node.tag->brace != xml_brace_none && node.shape != xml_shape_single) {
                            if (node.tag->block == xml_block_comment)
                                comment = element;
                            root = element;
                        }
                        break;

                    case xml_shape_close:
                        for (element = root; element; element = element->parent)
                            if (!xml_text_cmp(&element->name, &node.name))
                                break;
                        if (!element)
                            break;

                        root = element->parent;
                        break;
                }
                break;

            case xml_empty_node:
                break;

            case xml_text_node:
                if (!xml_text_is_empty(&node.content))
                    xml_element_append(root, &node);
                break;
        }
    }

    return 0;
}

/**
 *
 */
void xml_element_free(struct xml_element *node) {
    while (node) {
        struct xml_element *next = node->next;

        xml_element_free(node->children);
        xml_element_free(node->attribute);
        xml_text_free(node->content);
        xml_text_free(node->name);

        free(node);

        node = next;
    }
}

/**
 *
 */
void xml_free(struct xml *xml) {
    if (xml) {
        xml_element_free(xml->document.children);

        xml->document.children = NULL;
        xml->document.last = NULL;
    }
}

/**
 *
 */
void xml_init(struct xml *xml) {
    xml->document.type = xml_document_node;
}

/**
 *
 */
void xml_node_print(struct xml_element *node, FILE *out) {
    struct xml_element *attribute = NULL;
    switch (node->type) {
        case xml_attribute_node:
            fprintf(out, " ");
            fwrite(node->name.value, node->name.length, 1, out);
            if (node->shape != xml_shape_single) {
                fprintf(out, "=\"");
                fwrite(node->content.value, node->content.length, 1, out);
                fprintf(out, "\"");
            }
            break;
        case xml_document_node:
            break;

        case xml_element_node:
            fprintf(out, "%*s%02i:[", node->level, "", node->id);
            fwrite(node->name.value, node->name.length, 1, out);
            for (attribute = node->attribute; attribute; attribute = attribute->next) {
                xml_node_print(attribute, out);
            }
            fprintf(out, "]\n");
            break;

        case xml_empty_node:
            break;

        case xml_text_node:
            fprintf(out, "%*s\"", node->level, "");
            fwrite(node->content.value, node->content.length, 1, out);
            fprintf(out, "\"\n");
            break;
    }
}

/**
 *
 */
int xml_node_walk(struct xml_element *node, int (*callback)(struct xml_element *, int, void *), void *arg) {
    while (node) {
        struct xml_element *next = node->next;
        if (callback(node, 0, arg))
            return -1;
        if (xml_node_walk(node->children, callback, arg))
            return -1;
        if (callback(node, 1, arg))
            return -1;
        node = next;
    }
    return 0;
}

#define xml_seed_LENGTH(__max, __len, __inc) \
	if((__len += __inc) > __max) \
		return -1

struct __node_seed {
    char *buff;
    unsigned long max;
    int *len;
    char quote;
};

static int __callback_node_seed(struct xml_element *node, const int end, void *arg) {
    struct __node_seed *seed = arg;
    if (node) {
        switch (node->type) {
            case xml_attribute_node:
                if (end)
                    break;
                xml_seed_LENGTH(seed->max, *seed->len, 1);
                seed->buff[0] = ' ';
                seed->buff++;
                xml_seed_LENGTH(seed->max, *seed->len, node->name.length);
                memcpy(seed->buff, node->name.value, node->name.length);
                seed->buff += node->name.length;
                if (node->content.value) {
                    xml_seed_LENGTH(seed->max, *seed->len, 1);
                    seed->buff[0] = '=';
                    seed->buff++;
                    if (seed->quote) {
                        xml_seed_LENGTH(seed->max, *seed->len, 1);
                        seed->buff[0] = seed->quote;
                        seed->buff++;
                    }
                    xml_seed_LENGTH(seed->max, *seed->len, node->content.length);
                    memcpy(seed->buff, node->content.value, node->content.length);
                    seed->buff += node->content.length;
                    if (seed->quote) {
                        xml_seed_LENGTH(seed->max, *seed->len, 1);
                        seed->buff[0] = seed->quote;
                        seed->buff++;
                    }
                }
                break;

            case xml_document_node:
                break;

            case xml_element_node:
                if (end && node->shape == xml_shape_single)
                    break;
                xml_seed_LENGTH(seed->max, *seed->len, 1);
                seed->buff[0] = '<';
                seed->buff++;
                if (end) {
                    xml_seed_LENGTH(seed->max, *seed->len, 1);
                    seed->buff[0] = '/';
                    seed->buff++;
                }
                xml_seed_LENGTH(seed->max, *seed->len, node->name.length);
                memcpy(seed->buff, node->name.value, node->name.length);
                seed->buff += node->name.length;
                if (!end) {
                    if (xml_node_walk(node->attribute, __callback_node_seed, arg))
                        return -1;
                    if (node->shape == xml_shape_single) {
                        xml_seed_LENGTH(seed->max, *seed->len, 1);
                        seed->buff[0] = node->name.value[0] == '?' ? '?' : '/';
                        seed->buff++;
                    }
                }
                xml_seed_LENGTH(seed->max, *seed->len, 1);
                seed->buff[0] = '>';
                seed->buff++;
                break;

            case xml_empty_node:
                break;

            case xml_text_node:
                if (end)
                    break;
                xml_seed_LENGTH(seed->max, *seed->len, node->content.length);
                memcpy(seed->buff, node->content.value, node->content.length);
                seed->buff += node->content.length;
                break;
        }
    }
    return 0;
}

/**
 *
 */
int xml_seed(struct xml *xml, char buff[], const size_t max, const char quote) {
    if (!buff)
        return -1;

    int len = 0;

    struct __node_seed seed = {.buff = buff, .max = max, .len = &len, .quote = quote};

    if (xml_node_walk(xml->document.children, __callback_node_seed, &seed))
        return -1;

    return len;
}

static int __callback_node_print(struct xml_element *node, const int end, void *out) {
    if (end)
        return 0;
    if (!node->parent || !node->parent->tag || node->parent->tag->block != xml_block_comment)
        xml_node_print(node, out);
    return 0;
}

/**
 *
 */
void xml_dump(struct xml *xml, FILE *out) {
    xml_node_walk(xml->document.children, __callback_node_print, out);
}

/**
 *
 */
struct xml_element *xml_node_find(struct xml_element *node, const char *value, const int length) {
    while (node) {
        if (!xml_text_cmp2(node->name, xml_text_set(value, length)))
            break;
        node = node->next;
    }
    return node;
}

/**
 *
 */
struct xml_element *xml_node_lookup(struct xml_element *node, const char *value, const int length) {
    while (node) {
        if (node->children)
            node = node->children;
        else if (node->next)
            node = node->next;
        else {
            while (node && !node->next)
                node = node->parent;
            if (!node)
                break;
            node = node->next;
        }
        if (!xml_text_cmp2(node->name, xml_text_set(value, length)))
            return node;
    }
    return NULL;
}

/**
 *
 */
struct xml_element *xml_lookup(struct xml *xml, const char *value, const int length) {
    return xml_node_lookup(&xml->document, value, length);
}

/**
 *
 */
const struct xml_text *xml_entity_find(const char *entity, const int length) {
    const int code = *entity == '#' ? (int) strtol(entity + 1, 0, 0) : 0;

    for (int i = 0; i < sizeof(__xml_entity) / sizeof(*__xml_entity); i++) {
        if (code) {
            if (code == __xml_entity[i].number) {
                return &__xml_entity[i].symbol;
            }
        } else if (!xml_text_cmp2(__xml_entity[i].entity, xml_text_set(entity, length)))
            return &__xml_entity[i].symbol;
    }
    return &__xml_entity[0].symbol;
}

/**
 *
 */
int xml_text_merge(char *to, const int amount, const int length, const struct xml_text *from) {
    int l = amount;

    for (int i = 0; from && l < length && i < from->length; i++) {
        if (isspace(from->value[i])) {
            if (!l || to[l - 1] == ' ')
                continue;
            to[l] = ' ';
        } else if (from->value[i] == '&') {
            int j = i + 1;

            while (j < from->length) {
                if (from->value[j] != '#' && !isalnum(from->value[j]))
                    break;
                j++;
            }

            if (j < from->length && from->value[j] == ';') {
                l = l + xml_text_merge(to, l, length, xml_entity_find(from->value + i + 1, j - i - 1));
                i = j;
                continue;
            }
            to[l] = '&';
        } else
            to[l] = from->value[i];
        l++;
    }

    return l - amount;
}

struct __node_text {
    char *value;
    int length;
    int *amount;
};

static int __callback_node_text(struct xml_element *node, const int end, void *arg) {
    struct __node_text *text = arg;
    if (end)
        return 0;
    if (!node->parent || !node->parent->tag || node->parent->tag->block != xml_block_comment) {
        if (node->type == xml_text_node) {
            const int len = xml_text_merge(text->value, *text->amount, text->length, &node->content);
            *text->amount += len;
            text->length -= len;
        } else if (*text->amount && text->length) {
            text->value[*text->amount++] = ' ';
            text->length--;
        }
    }
    return 0;
}

/**
 *
 */
struct xml_text xml_node_text(struct xml_element *node, char *value, const int length) {
    int amount = 0;

    struct __node_text text = {.value = value, .length = length, .amount = &amount};

    xml_node_walk(node->children, __callback_node_text, &text);

    return xml_text_set(value, amount);
}

/**
 *
 */
unsigned xml_attribute_next(struct xml_element *attribute, const struct xml_text *content,
                            const unsigned offset, const char *reject) {
    int mode = 0;

    for (unsigned i = offset; i <= content->length; i++) {
        switch (mode) {
            case 0: // look for name begin
                if (isalpha(content->value[i])) {
                    attribute->name.value = content->value + i;
                    mode = 1;
                }
                break;

            case 1: // look for name end
                if (!isalpha(content->value[i]) && content->value[i] != '-' && content->value[i] != ':'
                    && content->value[i] != '_') {
                    attribute->type = xml_attribute_node;
                    attribute->shape = xml_shape_single;
                    attribute->name.length = content->value - attribute->name.value + i;

                    if (content->value[i] != '=')
                        mode = 2;
                    else
                        mode = 3;
                }
                break;

            case 2: // look for eq
                if (content->value[i] == '=')
                    mode = 3;
                else if (isalpha(content->value[i])) {
                    return i;
                }
                break;

            case 3: // look for content begin
                if (!isspace(content->value[i])) {
                    attribute->content.value = content->value + i;
                    switch (content->value[i]) {
                        case '\'':
                        case '\"':
                            attribute->content.value++;
                            break;
                        default:
                            break;
                    }
                    mode = 4;
                }
                attribute->shape = xml_shape_open;
                break;

            case 4: // look for content end
                if (reject) {
                    if (strchr(reject, content->value[i])) {
                        attribute->content.length = content->value + i - attribute->content.value;
                        mode = 5;
                        break;
                    }
                }
                switch (attribute->content.value[-1]) {
                    case '\'':
                    case '\"':
                        if (content->value[i] == attribute->content.value[-1] || i == content->length) {
                            attribute->content.length = content->value + i - attribute->content.value;
                            mode = 5;
                        }
                        break;

                    default:
                        if (isspace(content->value[i]) || i == content->length) {
                            attribute->content.length = content->value + i - attribute->content.value;
                            return i;
                        }
                        break;
                }
                break;

            default: // done
                return i;
        }
    }

    return content->length;
}

/**
 *
 */
struct xml_element *xml_node_new(const struct xml_element *node) {
    struct xml_element *add = malloc(sizeof(struct xml_element));
    if (add)
        if (node)
            memcpy(add, node, sizeof(struct xml_element));
    return add;
}

/**
 *
 */
void xml_node_compile(struct xml_element *node) {
    if (node->type == xml_element_node && !node->attribute && node->content.length) {
        unsigned offset = 0;

        while (offset < node->content.length) {
            struct xml_element attribute = xml_element_INITIALIZER;

            offset = xml_attribute_next(&attribute, &node->content, offset, NULL);

            if (attribute.type == xml_attribute_node) {
                attribute.parent = node;
                attribute.next = node->attribute;

                struct xml_element *add = xml_node_new(&attribute);
                if (add)
                    node->attribute = add;
            }
        }
    }
}

/**
 *
 */
struct xml_element *xml_element_init(struct xml_element *e, struct xml_element *parent, enum xml_type type,
                                     enum xml_shape shape, const char *name, unsigned nlen, const char *content,
                                     unsigned clen) {
    memset(e, 0, sizeof(xml_element_t));

    e->shape = shape;
    e->type = type;
    e->parent = parent;
    e->name = xml_text_set(name, nlen);
    e->content = xml_text_set(content, clen);
    return e;
}

/**
 *
 */
struct xml_element *xml_element_init2(struct xml_element *e, enum xml_type type, enum xml_shape shape,
                                      const struct xml_text *name, const struct xml_text *content) {
    memset(e, 0, sizeof(xml_element_t));

    e->type = type;
    e->shape = shape;
    if (name)
        e->name = *name;
    if (content)
        e->content = *content;

    return e;
}

/**
 *
 */
struct xml_element *xml_element_new(enum xml_type type, enum xml_shape shape, const struct xml_text *name,
                                    const struct xml_text *content) {
    struct xml_element *add = malloc(sizeof(struct xml_element));
    if (add) {
        return xml_element_init2(add, type, shape, name, content);
    }
    return add;
}

/**
 *
 */
unsigned xml_text_cpy(char *s, const int m, const struct xml_text t) {
    if (m) {
        const int l = m > t.length ? t.length : m - 1;

        memcpy(s, t.value, l);
        s[l] = 0;

        return l;
    }
    return 0;
}

/**
 *
 */
void xml_text_move(struct xml_text to, const struct xml_text from) {
    if (to.needfree)
        free((void *) to.value);

    to.value = from.value;
    to.length = from.length;
    to.needfree = from.needfree;
}

/**
 *
 */
char *xml_text_dup0(const char *c, const unsigned long n) {
    char *s = malloc(n + 1);

    memcpy(s, c, n);

    s[n] = 0;

    return s;
}

/**
 *
 */
struct xml_text xml_text_dup(const char *s, const unsigned long n) {
    const struct xml_text text = {
        .value = s && n ? xml_text_dup0(s, n) : NULL, .length = s && n ? n : 0, .needfree = 1
    };
    return text;
}

/*
 *
 */
struct xml_text xml_text_dup_string(const char *const *s) {
    return xml_text_dup(*s, strlen(*s));
}

/**
 *
 */
struct xml_text xml_text_cat(struct xml_text to, const struct xml_text from) {
    char *value = realloc((void *) to.value, to.length + from.length + 1);

    memcpy(value + to.length, from.value, from.length);

    value[to.length += from.length] = 0;
    to.value = value;

    return to;
}

/**
 *
 */
void xml_text_free(struct xml_text text) {
    if (text.needfree)
        free((void *) text.value);

    text.value = NULL;
    text.length = 0;
    text.needfree = 0;
}

/**
 *
 */
unsigned xml_text_length(const struct xml_text *text) {
    return text->length;
}

/**
 *
 */
const char *xml_text_value(const struct xml_text *text) {
    return text->value;
}

/**
 *
 */
int xml_text_int32(const struct xml_text *text) {
    return atoi(text->value);
}

/**
 *
 */
long long int xml_text_int64(const struct xml_text *text) {
    return strtoq(text->value, 0, 0);
}

/**
 *
 */
unsigned int xml_text_uint32(const struct xml_text *text) {
    return strtoul(text->value, 0, 0x0a);
}

/**
 *
 */
unsigned long long int xml_text_uint64(const struct xml_text *text) {
    return strtoull(text->value, 0, 0x0a);
}

/**
 *
 */
float xml_text_float(const struct xml_text *text) {
    return strtof(text->value, 0);
}

/**
 *
 */
double xml_text_double(const struct xml_text *text) {
    return strtod(text->value, 0);
}

/**
 *
 */
int xml_text_bool(const struct xml_text *text) {
    if (!xml_text_cmp4(text, "TRUE", 4) || !xml_text_cmp4(text, "YES", 3) || !xml_text_cmp4(text, "ON", 3)
        || xml_text_double(text))
        return 1;
    else
        return 0;
}

/**
 *
 */
struct xml_text xml_text_dup_int32(const int *value) {
    char text[256];

    return xml_text_dup(text, sprintf(text, "%d", *value));
}

/**
 *
 */
struct xml_text xml_text_dup_uint32(const unsigned int *value) {
    char text[256];

    return xml_text_dup(text, sprintf(text, "%u", *value));
}

/**
 *
 */
struct xml_text xml_text_dup_int64(const long long int *value) {
    char text[256];

    return xml_text_dup(text, sprintf(text, "%lld", *value));
}

/**
 *
 */
struct xml_text xml_text_dup_uint64(const unsigned long long int *value) {
    char text[256];

    return xml_text_dup(text, sprintf(text, "%llu", *value));
}

/**
 *
 */
struct xml_text xml_text_dup_float(const float *value) {
    char text[256];

    return xml_text_dup(text, sprintf(text, "%f", *value));
}

/**
 *
 */
struct xml_text xml_text_dup_double(const double *value) {
    char text[256];

    return xml_text_dup(text, sprintf(text, "%lf", *value));
}

/**
 *
 */
struct xml_text xml_text_dup_bool(const int *value) {
    char text[256];

    return xml_text_dup(text, sprintf(text, "%s", *value ? "true" : "false"));
}

/**
 *
 */
unsigned xml_text_cmp3(const struct xml_text *text, const char *sample) {
    return strncasecmp(text->value ? text->value : "", sample, text->length) ? text->length : sample[text->length];
}

/**
 *
 */
unsigned xml_text_cmp4(const struct xml_text *text, const char *sample, const unsigned length) {
    return text->length == length ? strncasecmp(text->value, sample, length) : text->length - length;
}

/**
 *
 */
struct xml_text xml_text_find_key2(const struct xml_text text, const struct xml_text key, const char *reject) {
    unsigned offset = 0;

    while (offset < text.length) {
        struct xml_element e = xml_element_INITIALIZER;

        offset = xml_attribute_next(&e, &text, offset, reject);
        if (!xml_text_cmp(&e.name, &key))
            return e.content;
    }
    return xml_text_set(0, 0);
}

#define islatin1(c)	( isalnum(c) || strchr("/?=#~'_-+:.()*!", c) )

/**
 *
 */
struct xml_text xml_text_escape2(const char *text, const unsigned length, char *buff, const int size) {
    int l = 0;
    int i = 0;

    for (; i < length && l < size; i++, l++) {
        if (!isalnum(0xFF & text[i]) && !ispunct(0xFF & text[i])) {
            sprintf(buff + l, "%%%02X", 0xFF & text[i]);
            l += 2;
            continue;
        }
        buff[l] = text[i];
    }
    return xml_text_set(buff, l);
}

/**
 *
 */
struct xml_text xml_text_unescape(const struct xml_text text, char *buff, int length) {
    int l = 0;
    char hex[3] = "00";

    for (int i = 0; i < text.length && l < length; i++, l++) {
        switch (text.value[i]) {
            case '%':
                hex[0] = text.value[++i];
                hex[1] = text.value[++i];
                buff[l] = strtol(hex, NULL, 16);
                break;

            case '+':
                buff[l] = ' ';
                break;

            default:
                buff[l] = text.value[i];
                break;
        }
    }
    return xml_text_set(buff, l);
}

/**
 *
 */
unsigned xml_text_cmp2(const struct xml_text a, const struct xml_text b) {
    return xml_text_cmp(&a, &b);
}

/**
 *
 */
struct xml_text xml_text_escape(const struct xml_text text, char *buff, const int size) {
    return xml_text_escape2(text.value, text.length, buff, size);
}

/**
 *
 */
struct xml_text xml_text_find_key(const char *text, const char *key, const char *reject) {
    return xml_text_find_key2(xml_text_set(text, strlen(text)), xml_text_set(key, strlen(key)), reject);
}

/**
 *
 */
struct xml_text xml_text_dup2(const struct xml_text text) {
    return xml_text_dup(text.value, text.length);
}

/**
 *
 */
struct xml_text xml_text_dup3(const char *text) {
    return xml_text_dup(text, text ? strlen(text) : 0);
}

/**
 *
 */
struct xml_element *xml_node_attribute(struct xml_element *node, const char *value, const int length) {
    if (node)
        return xml_node_find(node->attribute, value, length);
    return NULL;
}

/**
 *
 */
int xml_text_is_null(const struct xml_text *text) {
    return text->value && text->length ? 0 : 1;
}

/**
 *
 */
struct xml_text xml_text_decode(const struct xml_text text, char *buff, const int length) {
    return xml_text_set(buff, xml_text_merge(buff, 0, length, &text));
}
