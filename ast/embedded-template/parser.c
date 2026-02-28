#include "tree_sitter/parser.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#define LANGUAGE_VERSION 14
#define STATE_COUNT 29
#define LARGE_STATE_COUNT 6
#define SYMBOL_COUNT 31
#define ALIAS_COUNT 1
#define TOKEN_COUNT 21
#define EXTERNAL_TOKEN_COUNT 0
#define FIELD_COUNT 0
#define MAX_ALIAS_SEQUENCE_LENGTH 3
#define PRODUCTION_ID_COUNT 2

enum ts_symbol_identifiers {
  aux_sym_code_token1 = 1,
  anon_sym_PERCENT_PERCENT_GT = 2,
  aux_sym_content_token1 = 3,
  anon_sym_LT_PERCENT_PERCENT = 4,
  anon_sym_LT_PERCENT = 5,
  anon_sym_LT_PERCENT_ = 6,
  anon_sym_LT_PERCENT_PIPE = 7,
  anon_sym_LT_PERCENT_TILDE = 8,
  anon_sym_PERCENT_GT = 9,
  anon_sym_DASH_PERCENT_GT = 10,
  anon_sym__PERCENT_GT = 11,
  anon_sym_LT_PERCENT_EQ = 12,
  anon_sym_LT_PERCENT_EQ_EQ = 13,
  anon_sym_LT_PERCENT_PIPE_EQ = 14,
  anon_sym_LT_PERCENT_PIPE_EQ_EQ = 15,
  anon_sym_LT_PERCENT_DASH = 16,
  anon_sym_xyz = 17,
  anon_sym_EQ_PERCENT_GT = 18,
  anon_sym_LT_PERCENT_POUND = 19,
  anon_sym_LT_PERCENTgraphql = 20,
  sym_template = 21,
  sym_code = 22,
  sym_content = 23,
  sym_directive = 24,
  sym_output_directive = 25,
  sym_comment_directive = 26,
  sym_graphql_directive = 27,
  aux_sym_template_repeat1 = 28,
  aux_sym_code_repeat1 = 29,
  aux_sym_content_repeat1 = 30,
  alias_sym_comment = 31,
};

static const char * const ts_symbol_names[] = {
  [ts_builtin_sym_end] = "end",
  [aux_sym_code_token1] = "code_token1",
  [anon_sym_PERCENT_PERCENT_GT] = "%%>",
  [aux_sym_content_token1] = "content_token1",
  [anon_sym_LT_PERCENT_PERCENT] = "<%%",
  [anon_sym_LT_PERCENT] = "<%",
  [anon_sym_LT_PERCENT_] = "<%_",
  [anon_sym_LT_PERCENT_PIPE] = "<%|",
  [anon_sym_LT_PERCENT_TILDE] = "<%~",
  [anon_sym_PERCENT_GT] = "%>",
  [anon_sym_DASH_PERCENT_GT] = "-%>",
  [anon_sym__PERCENT_GT] = "_%>",
  [anon_sym_LT_PERCENT_EQ] = "<%=",
  [anon_sym_LT_PERCENT_EQ_EQ] = "<%==",
  [anon_sym_LT_PERCENT_PIPE_EQ] = "<%|=",
  [anon_sym_LT_PERCENT_PIPE_EQ_EQ] = "<%|==",
  [anon_sym_LT_PERCENT_DASH] = "<%-",
  [anon_sym_xyz] = "xyz",
  [anon_sym_EQ_PERCENT_GT] = "=%>",
  [anon_sym_LT_PERCENT_POUND] = "<%#",
  [anon_sym_LT_PERCENTgraphql] = "<%graphql",
  [sym_template] = "template",
  [sym_code] = "code",
  [sym_content] = "content",
  [sym_directive] = "directive",
  [sym_output_directive] = "output_directive",
  [sym_comment_directive] = "comment_directive",
  [sym_graphql_directive] = "graphql_directive",
  [aux_sym_template_repeat1] = "template_repeat1",
  [aux_sym_code_repeat1] = "code_repeat1",
  [aux_sym_content_repeat1] = "content_repeat1",
  [alias_sym_comment] = "comment",
};

static const TSSymbol ts_symbol_map[] = {
  [ts_builtin_sym_end] = ts_builtin_sym_end,
  [aux_sym_code_token1] = aux_sym_code_token1,
  [anon_sym_PERCENT_PERCENT_GT] = anon_sym_PERCENT_PERCENT_GT,
  [aux_sym_content_token1] = aux_sym_content_token1,
  [anon_sym_LT_PERCENT_PERCENT] = anon_sym_LT_PERCENT_PERCENT,
  [anon_sym_LT_PERCENT] = anon_sym_LT_PERCENT,
  [anon_sym_LT_PERCENT_] = anon_sym_LT_PERCENT_,
  [anon_sym_LT_PERCENT_PIPE] = anon_sym_LT_PERCENT_PIPE,
  [anon_sym_LT_PERCENT_TILDE] = anon_sym_LT_PERCENT_TILDE,
  [anon_sym_PERCENT_GT] = anon_sym_PERCENT_GT,
  [anon_sym_DASH_PERCENT_GT] = anon_sym_DASH_PERCENT_GT,
  [anon_sym__PERCENT_GT] = anon_sym__PERCENT_GT,
  [anon_sym_LT_PERCENT_EQ] = anon_sym_LT_PERCENT_EQ,
  [anon_sym_LT_PERCENT_EQ_EQ] = anon_sym_LT_PERCENT_EQ_EQ,
  [anon_sym_LT_PERCENT_PIPE_EQ] = anon_sym_LT_PERCENT_PIPE_EQ,
  [anon_sym_LT_PERCENT_PIPE_EQ_EQ] = anon_sym_LT_PERCENT_PIPE_EQ_EQ,
  [anon_sym_LT_PERCENT_DASH] = anon_sym_LT_PERCENT_DASH,
  [anon_sym_xyz] = anon_sym_xyz,
  [anon_sym_EQ_PERCENT_GT] = anon_sym_EQ_PERCENT_GT,
  [anon_sym_LT_PERCENT_POUND] = anon_sym_LT_PERCENT_POUND,
  [anon_sym_LT_PERCENTgraphql] = anon_sym_LT_PERCENTgraphql,
  [sym_template] = sym_template,
  [sym_code] = sym_code,
  [sym_content] = sym_content,
  [sym_directive] = sym_directive,
  [sym_output_directive] = sym_output_directive,
  [sym_comment_directive] = sym_comment_directive,
  [sym_graphql_directive] = sym_graphql_directive,
  [aux_sym_template_repeat1] = aux_sym_template_repeat1,
  [aux_sym_code_repeat1] = aux_sym_code_repeat1,
  [aux_sym_content_repeat1] = aux_sym_content_repeat1,
  [alias_sym_comment] = alias_sym_comment,
};

static const TSSymbolMetadata ts_symbol_metadata[] = {
  [ts_builtin_sym_end] = {
    .visible = false,
    .named = true,
  },
  [aux_sym_code_token1] = {
    .visible = false,
    .named = false,
  },
  [anon_sym_PERCENT_PERCENT_GT] = {
    .visible = true,
    .named = false,
  },
  [aux_sym_content_token1] = {
    .visible = false,
    .named = false,
  },
  [anon_sym_LT_PERCENT_PERCENT] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_LT_PERCENT] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_LT_PERCENT_] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_LT_PERCENT_PIPE] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_LT_PERCENT_TILDE] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_PERCENT_GT] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_DASH_PERCENT_GT] = {
    .visible = true,
    .named = false,
  },
  [anon_sym__PERCENT_GT] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_LT_PERCENT_EQ] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_LT_PERCENT_EQ_EQ] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_LT_PERCENT_PIPE_EQ] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_LT_PERCENT_PIPE_EQ_EQ] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_LT_PERCENT_DASH] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_xyz] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_EQ_PERCENT_GT] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_LT_PERCENT_POUND] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_LT_PERCENTgraphql] = {
    .visible = true,
    .named = false,
  },
  [sym_template] = {
    .visible = true,
    .named = true,
  },
  [sym_code] = {
    .visible = true,
    .named = true,
  },
  [sym_content] = {
    .visible = true,
    .named = true,
  },
  [sym_directive] = {
    .visible = true,
    .named = true,
  },
  [sym_output_directive] = {
    .visible = true,
    .named = true,
  },
  [sym_comment_directive] = {
    .visible = true,
    .named = true,
  },
  [sym_graphql_directive] = {
    .visible = true,
    .named = true,
  },
  [aux_sym_template_repeat1] = {
    .visible = false,
    .named = false,
  },
  [aux_sym_code_repeat1] = {
    .visible = false,
    .named = false,
  },
  [aux_sym_content_repeat1] = {
    .visible = false,
    .named = false,
  },
  [alias_sym_comment] = {
    .visible = true,
    .named = true,
  },
};

static const TSSymbol ts_alias_sequences[PRODUCTION_ID_COUNT][MAX_ALIAS_SEQUENCE_LENGTH] = {
  [0] = {0},
  [1] = {
    [1] = alias_sym_comment,
  },
};

static const uint16_t ts_non_terminal_alias_map[] = {
  sym_code, 2,
    sym_code,
    alias_sym_comment,
  0,
};

static const TSStateId ts_primary_state_ids[STATE_COUNT] = {
  [0] = 0,
  [1] = 1,
  [2] = 2,
  [3] = 3,
  [4] = 4,
  [5] = 5,
  [6] = 6,
  [7] = 7,
  [8] = 8,
  [9] = 9,
  [10] = 10,
  [11] = 11,
  [12] = 12,
  [13] = 13,
  [14] = 14,
  [15] = 15,
  [16] = 16,
  [17] = 17,
  [18] = 17,
  [19] = 16,
  [20] = 20,
  [21] = 21,
  [22] = 16,
  [23] = 17,
  [24] = 24,
  [25] = 25,
  [26] = 26,
  [27] = 27,
  [28] = 28,
};

static bool ts_lex(TSLexer *lexer, TSStateId state) {
  START_LEXER();
  eof = lexer->eof(lexer);
  switch (state) {
    case 0:
      if (eof) ADVANCE(22);
      if (lookahead == '%') ADVANCE(1);
      if (lookahead == '-') ADVANCE(6);
      if (lookahead == '<') ADVANCE(2);
      if (lookahead == '=') ADVANCE(7);
      if (lookahead == '_') ADVANCE(8);
      if (lookahead == 'x') ADVANCE(19);
      END_STATE();
    case 1:
      if (lookahead == '%') ADVANCE(9);
      if (lookahead == '>') ADVANCE(39);
      END_STATE();
    case 2:
      if (lookahead == '%') ADVANCE(35);
      END_STATE();
    case 3:
      if (lookahead == '%') ADVANCE(24);
      if (lookahead == '-') ADVANCE(25);
      if (lookahead == '=') ADVANCE(23);
      if (lookahead == '_') ADVANCE(27);
      if (lookahead != 0) ADVANCE(28);
      END_STATE();
    case 4:
      if (lookahead == '%') ADVANCE(24);
      if (lookahead == '-') ADVANCE(25);
      if (lookahead == '=') ADVANCE(26);
      if (lookahead == '_') ADVANCE(23);
      if (lookahead != 0) ADVANCE(28);
      END_STATE();
    case 5:
      if (lookahead == '%') ADVANCE(24);
      if (lookahead == '-' ||
          lookahead == '=' ||
          lookahead == '_') ADVANCE(23);
      if (lookahead != 0) ADVANCE(28);
      END_STATE();
    case 6:
      if (lookahead == '%') ADVANCE(10);
      END_STATE();
    case 7:
      if (lookahead == '%') ADVANCE(11);
      END_STATE();
    case 8:
      if (lookahead == '%') ADVANCE(12);
      END_STATE();
    case 9:
      if (lookahead == '>') ADVANCE(29);
      END_STATE();
    case 10:
      if (lookahead == '>') ADVANCE(40);
      END_STATE();
    case 11:
      if (lookahead == '>') ADVANCE(49);
      END_STATE();
    case 12:
      if (lookahead == '>') ADVANCE(41);
      END_STATE();
    case 13:
      if (lookahead == 'a') ADVANCE(16);
      END_STATE();
    case 14:
      if (lookahead == 'h') ADVANCE(17);
      END_STATE();
    case 15:
      if (lookahead == 'l') ADVANCE(51);
      END_STATE();
    case 16:
      if (lookahead == 'p') ADVANCE(14);
      END_STATE();
    case 17:
      if (lookahead == 'q') ADVANCE(15);
      END_STATE();
    case 18:
      if (lookahead == 'r') ADVANCE(13);
      END_STATE();
    case 19:
      if (lookahead == 'y') ADVANCE(20);
      END_STATE();
    case 20:
      if (lookahead == 'z') ADVANCE(47);
      END_STATE();
    case 21:
      if (eof) ADVANCE(22);
      if (lookahead == '<') ADVANCE(30);
      if (lookahead == 'x') ADVANCE(31);
      if (lookahead != 0) ADVANCE(33);
      END_STATE();
    case 22:
      ACCEPT_TOKEN(ts_builtin_sym_end);
      END_STATE();
    case 23:
      ACCEPT_TOKEN(aux_sym_code_token1);
      END_STATE();
    case 24:
      ACCEPT_TOKEN(aux_sym_code_token1);
      if (lookahead == '%') ADVANCE(9);
      if (lookahead == '>') ADVANCE(39);
      END_STATE();
    case 25:
      ACCEPT_TOKEN(aux_sym_code_token1);
      if (lookahead == '%') ADVANCE(10);
      END_STATE();
    case 26:
      ACCEPT_TOKEN(aux_sym_code_token1);
      if (lookahead == '%') ADVANCE(11);
      END_STATE();
    case 27:
      ACCEPT_TOKEN(aux_sym_code_token1);
      if (lookahead == '%') ADVANCE(12);
      END_STATE();
    case 28:
      ACCEPT_TOKEN(aux_sym_code_token1);
      if (lookahead != 0 &&
          lookahead != '%' &&
          lookahead != '-' &&
          lookahead != '=' &&
          lookahead != '_') ADVANCE(28);
      END_STATE();
    case 29:
      ACCEPT_TOKEN(anon_sym_PERCENT_PERCENT_GT);
      END_STATE();
    case 30:
      ACCEPT_TOKEN(aux_sym_content_token1);
      if (lookahead == '%') ADVANCE(35);
      END_STATE();
    case 31:
      ACCEPT_TOKEN(aux_sym_content_token1);
      if (lookahead == 'y') ADVANCE(32);
      if (lookahead != 0 &&
          lookahead != '<') ADVANCE(33);
      END_STATE();
    case 32:
      ACCEPT_TOKEN(aux_sym_content_token1);
      if (lookahead == 'z') ADVANCE(48);
      if (lookahead != 0 &&
          lookahead != '<') ADVANCE(33);
      END_STATE();
    case 33:
      ACCEPT_TOKEN(aux_sym_content_token1);
      if (lookahead != 0 &&
          lookahead != '<') ADVANCE(33);
      END_STATE();
    case 34:
      ACCEPT_TOKEN(anon_sym_LT_PERCENT_PERCENT);
      END_STATE();
    case 35:
      ACCEPT_TOKEN(anon_sym_LT_PERCENT);
      ADVANCE_MAP(
        '#', 50,
        '%', 34,
        '-', 46,
        '=', 42,
        '_', 36,
        'g', 18,
        '|', 37,
        '~', 38,
      );
      END_STATE();
    case 36:
      ACCEPT_TOKEN(anon_sym_LT_PERCENT_);
      END_STATE();
    case 37:
      ACCEPT_TOKEN(anon_sym_LT_PERCENT_PIPE);
      if (lookahead == '=') ADVANCE(44);
      END_STATE();
    case 38:
      ACCEPT_TOKEN(anon_sym_LT_PERCENT_TILDE);
      END_STATE();
    case 39:
      ACCEPT_TOKEN(anon_sym_PERCENT_GT);
      END_STATE();
    case 40:
      ACCEPT_TOKEN(anon_sym_DASH_PERCENT_GT);
      END_STATE();
    case 41:
      ACCEPT_TOKEN(anon_sym__PERCENT_GT);
      END_STATE();
    case 42:
      ACCEPT_TOKEN(anon_sym_LT_PERCENT_EQ);
      if (lookahead == '=') ADVANCE(43);
      END_STATE();
    case 43:
      ACCEPT_TOKEN(anon_sym_LT_PERCENT_EQ_EQ);
      END_STATE();
    case 44:
      ACCEPT_TOKEN(anon_sym_LT_PERCENT_PIPE_EQ);
      if (lookahead == '=') ADVANCE(45);
      END_STATE();
    case 45:
      ACCEPT_TOKEN(anon_sym_LT_PERCENT_PIPE_EQ_EQ);
      END_STATE();
    case 46:
      ACCEPT_TOKEN(anon_sym_LT_PERCENT_DASH);
      END_STATE();
    case 47:
      ACCEPT_TOKEN(anon_sym_xyz);
      END_STATE();
    case 48:
      ACCEPT_TOKEN(anon_sym_xyz);
      if (lookahead != 0 &&
          lookahead != '<') ADVANCE(33);
      END_STATE();
    case 49:
      ACCEPT_TOKEN(anon_sym_EQ_PERCENT_GT);
      END_STATE();
    case 50:
      ACCEPT_TOKEN(anon_sym_LT_PERCENT_POUND);
      END_STATE();
    case 51:
      ACCEPT_TOKEN(anon_sym_LT_PERCENTgraphql);
      END_STATE();
    default:
      return false;
  }
}

static const TSLexMode ts_lex_modes[STATE_COUNT] = {
  [0] = {.lex_state = 0},
  [1] = {.lex_state = 21},
  [2] = {.lex_state = 21},
  [3] = {.lex_state = 21},
  [4] = {.lex_state = 21},
  [5] = {.lex_state = 21},
  [6] = {.lex_state = 21},
  [7] = {.lex_state = 21},
  [8] = {.lex_state = 21},
  [9] = {.lex_state = 21},
  [10] = {.lex_state = 21},
  [11] = {.lex_state = 21},
  [12] = {.lex_state = 21},
  [13] = {.lex_state = 21},
  [14] = {.lex_state = 4},
  [15] = {.lex_state = 3},
  [16] = {.lex_state = 3},
  [17] = {.lex_state = 3},
  [18] = {.lex_state = 4},
  [19] = {.lex_state = 4},
  [20] = {.lex_state = 5},
  [21] = {.lex_state = 5},
  [22] = {.lex_state = 5},
  [23] = {.lex_state = 5},
  [24] = {.lex_state = 0},
  [25] = {.lex_state = 0},
  [26] = {.lex_state = 0},
  [27] = {.lex_state = 0},
  [28] = {.lex_state = 0},
};

static const uint16_t ts_parse_table[LARGE_STATE_COUNT][SYMBOL_COUNT] = {
  [0] = {
    [ts_builtin_sym_end] = ACTIONS(1),
    [anon_sym_PERCENT_PERCENT_GT] = ACTIONS(1),
    [anon_sym_LT_PERCENT_PERCENT] = ACTIONS(1),
    [anon_sym_LT_PERCENT] = ACTIONS(1),
    [anon_sym_LT_PERCENT_] = ACTIONS(1),
    [anon_sym_LT_PERCENT_PIPE] = ACTIONS(1),
    [anon_sym_LT_PERCENT_TILDE] = ACTIONS(1),
    [anon_sym_PERCENT_GT] = ACTIONS(1),
    [anon_sym_DASH_PERCENT_GT] = ACTIONS(1),
    [anon_sym__PERCENT_GT] = ACTIONS(1),
    [anon_sym_LT_PERCENT_EQ] = ACTIONS(1),
    [anon_sym_LT_PERCENT_EQ_EQ] = ACTIONS(1),
    [anon_sym_LT_PERCENT_PIPE_EQ] = ACTIONS(1),
    [anon_sym_LT_PERCENT_PIPE_EQ_EQ] = ACTIONS(1),
    [anon_sym_LT_PERCENT_DASH] = ACTIONS(1),
    [anon_sym_xyz] = ACTIONS(1),
    [anon_sym_EQ_PERCENT_GT] = ACTIONS(1),
    [anon_sym_LT_PERCENT_POUND] = ACTIONS(1),
    [anon_sym_LT_PERCENTgraphql] = ACTIONS(1),
  },
  [1] = {
    [sym_template] = STATE(26),
    [sym_content] = STATE(2),
    [sym_directive] = STATE(2),
    [sym_output_directive] = STATE(2),
    [sym_comment_directive] = STATE(2),
    [sym_graphql_directive] = STATE(2),
    [aux_sym_template_repeat1] = STATE(2),
    [aux_sym_content_repeat1] = STATE(4),
    [ts_builtin_sym_end] = ACTIONS(3),
    [aux_sym_content_token1] = ACTIONS(5),
    [anon_sym_LT_PERCENT_PERCENT] = ACTIONS(7),
    [anon_sym_LT_PERCENT] = ACTIONS(9),
    [anon_sym_LT_PERCENT_] = ACTIONS(11),
    [anon_sym_LT_PERCENT_PIPE] = ACTIONS(9),
    [anon_sym_LT_PERCENT_TILDE] = ACTIONS(11),
    [anon_sym_LT_PERCENT_EQ] = ACTIONS(13),
    [anon_sym_LT_PERCENT_EQ_EQ] = ACTIONS(15),
    [anon_sym_LT_PERCENT_PIPE_EQ] = ACTIONS(13),
    [anon_sym_LT_PERCENT_PIPE_EQ_EQ] = ACTIONS(15),
    [anon_sym_LT_PERCENT_DASH] = ACTIONS(15),
    [anon_sym_xyz] = ACTIONS(13),
    [anon_sym_LT_PERCENT_POUND] = ACTIONS(17),
    [anon_sym_LT_PERCENTgraphql] = ACTIONS(19),
  },
  [2] = {
    [sym_content] = STATE(3),
    [sym_directive] = STATE(3),
    [sym_output_directive] = STATE(3),
    [sym_comment_directive] = STATE(3),
    [sym_graphql_directive] = STATE(3),
    [aux_sym_template_repeat1] = STATE(3),
    [aux_sym_content_repeat1] = STATE(4),
    [ts_builtin_sym_end] = ACTIONS(21),
    [aux_sym_content_token1] = ACTIONS(5),
    [anon_sym_LT_PERCENT_PERCENT] = ACTIONS(7),
    [anon_sym_LT_PERCENT] = ACTIONS(9),
    [anon_sym_LT_PERCENT_] = ACTIONS(11),
    [anon_sym_LT_PERCENT_PIPE] = ACTIONS(9),
    [anon_sym_LT_PERCENT_TILDE] = ACTIONS(11),
    [anon_sym_LT_PERCENT_EQ] = ACTIONS(13),
    [anon_sym_LT_PERCENT_EQ_EQ] = ACTIONS(15),
    [anon_sym_LT_PERCENT_PIPE_EQ] = ACTIONS(13),
    [anon_sym_LT_PERCENT_PIPE_EQ_EQ] = ACTIONS(15),
    [anon_sym_LT_PERCENT_DASH] = ACTIONS(15),
    [anon_sym_xyz] = ACTIONS(13),
    [anon_sym_LT_PERCENT_POUND] = ACTIONS(17),
    [anon_sym_LT_PERCENTgraphql] = ACTIONS(19),
  },
  [3] = {
    [sym_content] = STATE(3),
    [sym_directive] = STATE(3),
    [sym_output_directive] = STATE(3),
    [sym_comment_directive] = STATE(3),
    [sym_graphql_directive] = STATE(3),
    [aux_sym_template_repeat1] = STATE(3),
    [aux_sym_content_repeat1] = STATE(4),
    [ts_builtin_sym_end] = ACTIONS(23),
    [aux_sym_content_token1] = ACTIONS(25),
    [anon_sym_LT_PERCENT_PERCENT] = ACTIONS(28),
    [anon_sym_LT_PERCENT] = ACTIONS(31),
    [anon_sym_LT_PERCENT_] = ACTIONS(34),
    [anon_sym_LT_PERCENT_PIPE] = ACTIONS(31),
    [anon_sym_LT_PERCENT_TILDE] = ACTIONS(34),
    [anon_sym_LT_PERCENT_EQ] = ACTIONS(37),
    [anon_sym_LT_PERCENT_EQ_EQ] = ACTIONS(40),
    [anon_sym_LT_PERCENT_PIPE_EQ] = ACTIONS(37),
    [anon_sym_LT_PERCENT_PIPE_EQ_EQ] = ACTIONS(40),
    [anon_sym_LT_PERCENT_DASH] = ACTIONS(40),
    [anon_sym_xyz] = ACTIONS(37),
    [anon_sym_LT_PERCENT_POUND] = ACTIONS(43),
    [anon_sym_LT_PERCENTgraphql] = ACTIONS(46),
  },
  [4] = {
    [aux_sym_content_repeat1] = STATE(5),
    [ts_builtin_sym_end] = ACTIONS(49),
    [aux_sym_content_token1] = ACTIONS(51),
    [anon_sym_LT_PERCENT_PERCENT] = ACTIONS(53),
    [anon_sym_LT_PERCENT] = ACTIONS(55),
    [anon_sym_LT_PERCENT_] = ACTIONS(49),
    [anon_sym_LT_PERCENT_PIPE] = ACTIONS(55),
    [anon_sym_LT_PERCENT_TILDE] = ACTIONS(49),
    [anon_sym_LT_PERCENT_EQ] = ACTIONS(55),
    [anon_sym_LT_PERCENT_EQ_EQ] = ACTIONS(49),
    [anon_sym_LT_PERCENT_PIPE_EQ] = ACTIONS(55),
    [anon_sym_LT_PERCENT_PIPE_EQ_EQ] = ACTIONS(49),
    [anon_sym_LT_PERCENT_DASH] = ACTIONS(49),
    [anon_sym_xyz] = ACTIONS(55),
    [anon_sym_LT_PERCENT_POUND] = ACTIONS(49),
    [anon_sym_LT_PERCENTgraphql] = ACTIONS(49),
  },
  [5] = {
    [aux_sym_content_repeat1] = STATE(5),
    [ts_builtin_sym_end] = ACTIONS(57),
    [aux_sym_content_token1] = ACTIONS(59),
    [anon_sym_LT_PERCENT_PERCENT] = ACTIONS(62),
    [anon_sym_LT_PERCENT] = ACTIONS(65),
    [anon_sym_LT_PERCENT_] = ACTIONS(57),
    [anon_sym_LT_PERCENT_PIPE] = ACTIONS(65),
    [anon_sym_LT_PERCENT_TILDE] = ACTIONS(57),
    [anon_sym_LT_PERCENT_EQ] = ACTIONS(65),
    [anon_sym_LT_PERCENT_EQ_EQ] = ACTIONS(57),
    [anon_sym_LT_PERCENT_PIPE_EQ] = ACTIONS(65),
    [anon_sym_LT_PERCENT_PIPE_EQ_EQ] = ACTIONS(57),
    [anon_sym_LT_PERCENT_DASH] = ACTIONS(57),
    [anon_sym_xyz] = ACTIONS(65),
    [anon_sym_LT_PERCENT_POUND] = ACTIONS(57),
    [anon_sym_LT_PERCENTgraphql] = ACTIONS(57),
  },
};

static const uint16_t ts_small_parse_table[] = {
  [0] = 2,
    ACTIONS(69), 6,
      aux_sym_content_token1,
      anon_sym_LT_PERCENT,
      anon_sym_LT_PERCENT_PIPE,
      anon_sym_LT_PERCENT_EQ,
      anon_sym_LT_PERCENT_PIPE_EQ,
      anon_sym_xyz,
    ACTIONS(67), 9,
      ts_builtin_sym_end,
      anon_sym_LT_PERCENT_PERCENT,
      anon_sym_LT_PERCENT_,
      anon_sym_LT_PERCENT_TILDE,
      anon_sym_LT_PERCENT_EQ_EQ,
      anon_sym_LT_PERCENT_PIPE_EQ_EQ,
      anon_sym_LT_PERCENT_DASH,
      anon_sym_LT_PERCENT_POUND,
      anon_sym_LT_PERCENTgraphql,
  [20] = 2,
    ACTIONS(73), 6,
      aux_sym_content_token1,
      anon_sym_LT_PERCENT,
      anon_sym_LT_PERCENT_PIPE,
      anon_sym_LT_PERCENT_EQ,
      anon_sym_LT_PERCENT_PIPE_EQ,
      anon_sym_xyz,
    ACTIONS(71), 9,
      ts_builtin_sym_end,
      anon_sym_LT_PERCENT_PERCENT,
      anon_sym_LT_PERCENT_,
      anon_sym_LT_PERCENT_TILDE,
      anon_sym_LT_PERCENT_EQ_EQ,
      anon_sym_LT_PERCENT_PIPE_EQ_EQ,
      anon_sym_LT_PERCENT_DASH,
      anon_sym_LT_PERCENT_POUND,
      anon_sym_LT_PERCENTgraphql,
  [40] = 2,
    ACTIONS(77), 6,
      aux_sym_content_token1,
      anon_sym_LT_PERCENT,
      anon_sym_LT_PERCENT_PIPE,
      anon_sym_LT_PERCENT_EQ,
      anon_sym_LT_PERCENT_PIPE_EQ,
      anon_sym_xyz,
    ACTIONS(75), 9,
      ts_builtin_sym_end,
      anon_sym_LT_PERCENT_PERCENT,
      anon_sym_LT_PERCENT_,
      anon_sym_LT_PERCENT_TILDE,
      anon_sym_LT_PERCENT_EQ_EQ,
      anon_sym_LT_PERCENT_PIPE_EQ_EQ,
      anon_sym_LT_PERCENT_DASH,
      anon_sym_LT_PERCENT_POUND,
      anon_sym_LT_PERCENTgraphql,
  [60] = 2,
    ACTIONS(81), 6,
      aux_sym_content_token1,
      anon_sym_LT_PERCENT,
      anon_sym_LT_PERCENT_PIPE,
      anon_sym_LT_PERCENT_EQ,
      anon_sym_LT_PERCENT_PIPE_EQ,
      anon_sym_xyz,
    ACTIONS(79), 9,
      ts_builtin_sym_end,
      anon_sym_LT_PERCENT_PERCENT,
      anon_sym_LT_PERCENT_,
      anon_sym_LT_PERCENT_TILDE,
      anon_sym_LT_PERCENT_EQ_EQ,
      anon_sym_LT_PERCENT_PIPE_EQ_EQ,
      anon_sym_LT_PERCENT_DASH,
      anon_sym_LT_PERCENT_POUND,
      anon_sym_LT_PERCENTgraphql,
  [80] = 2,
    ACTIONS(85), 6,
      aux_sym_content_token1,
      anon_sym_LT_PERCENT,
      anon_sym_LT_PERCENT_PIPE,
      anon_sym_LT_PERCENT_EQ,
      anon_sym_LT_PERCENT_PIPE_EQ,
      anon_sym_xyz,
    ACTIONS(83), 9,
      ts_builtin_sym_end,
      anon_sym_LT_PERCENT_PERCENT,
      anon_sym_LT_PERCENT_,
      anon_sym_LT_PERCENT_TILDE,
      anon_sym_LT_PERCENT_EQ_EQ,
      anon_sym_LT_PERCENT_PIPE_EQ_EQ,
      anon_sym_LT_PERCENT_DASH,
      anon_sym_LT_PERCENT_POUND,
      anon_sym_LT_PERCENTgraphql,
  [100] = 2,
    ACTIONS(89), 6,
      aux_sym_content_token1,
      anon_sym_LT_PERCENT,
      anon_sym_LT_PERCENT_PIPE,
      anon_sym_LT_PERCENT_EQ,
      anon_sym_LT_PERCENT_PIPE_EQ,
      anon_sym_xyz,
    ACTIONS(87), 9,
      ts_builtin_sym_end,
      anon_sym_LT_PERCENT_PERCENT,
      anon_sym_LT_PERCENT_,
      anon_sym_LT_PERCENT_TILDE,
      anon_sym_LT_PERCENT_EQ_EQ,
      anon_sym_LT_PERCENT_PIPE_EQ_EQ,
      anon_sym_LT_PERCENT_DASH,
      anon_sym_LT_PERCENT_POUND,
      anon_sym_LT_PERCENTgraphql,
  [120] = 2,
    ACTIONS(93), 6,
      aux_sym_content_token1,
      anon_sym_LT_PERCENT,
      anon_sym_LT_PERCENT_PIPE,
      anon_sym_LT_PERCENT_EQ,
      anon_sym_LT_PERCENT_PIPE_EQ,
      anon_sym_xyz,
    ACTIONS(91), 9,
      ts_builtin_sym_end,
      anon_sym_LT_PERCENT_PERCENT,
      anon_sym_LT_PERCENT_,
      anon_sym_LT_PERCENT_TILDE,
      anon_sym_LT_PERCENT_EQ_EQ,
      anon_sym_LT_PERCENT_PIPE_EQ_EQ,
      anon_sym_LT_PERCENT_DASH,
      anon_sym_LT_PERCENT_POUND,
      anon_sym_LT_PERCENTgraphql,
  [140] = 2,
    ACTIONS(97), 6,
      aux_sym_content_token1,
      anon_sym_LT_PERCENT,
      anon_sym_LT_PERCENT_PIPE,
      anon_sym_LT_PERCENT_EQ,
      anon_sym_LT_PERCENT_PIPE_EQ,
      anon_sym_xyz,
    ACTIONS(95), 9,
      ts_builtin_sym_end,
      anon_sym_LT_PERCENT_PERCENT,
      anon_sym_LT_PERCENT_,
      anon_sym_LT_PERCENT_TILDE,
      anon_sym_LT_PERCENT_EQ_EQ,
      anon_sym_LT_PERCENT_PIPE_EQ_EQ,
      anon_sym_LT_PERCENT_DASH,
      anon_sym_LT_PERCENT_POUND,
      anon_sym_LT_PERCENTgraphql,
  [160] = 5,
    ACTIONS(99), 1,
      aux_sym_code_token1,
    ACTIONS(101), 1,
      anon_sym_PERCENT_PERCENT_GT,
    STATE(19), 1,
      aux_sym_code_repeat1,
    STATE(25), 1,
      sym_code,
    ACTIONS(103), 3,
      anon_sym_PERCENT_GT,
      anon_sym_DASH_PERCENT_GT,
      anon_sym_EQ_PERCENT_GT,
  [178] = 5,
    ACTIONS(105), 1,
      aux_sym_code_token1,
    ACTIONS(107), 1,
      anon_sym_PERCENT_PERCENT_GT,
    STATE(16), 1,
      aux_sym_code_repeat1,
    STATE(24), 1,
      sym_code,
    ACTIONS(109), 3,
      anon_sym_PERCENT_GT,
      anon_sym_DASH_PERCENT_GT,
      anon_sym__PERCENT_GT,
  [196] = 4,
    ACTIONS(111), 1,
      aux_sym_code_token1,
    ACTIONS(113), 1,
      anon_sym_PERCENT_PERCENT_GT,
    STATE(17), 1,
      aux_sym_code_repeat1,
    ACTIONS(115), 3,
      anon_sym_PERCENT_GT,
      anon_sym_DASH_PERCENT_GT,
      anon_sym__PERCENT_GT,
  [211] = 4,
    ACTIONS(117), 1,
      aux_sym_code_token1,
    ACTIONS(120), 1,
      anon_sym_PERCENT_PERCENT_GT,
    STATE(17), 1,
      aux_sym_code_repeat1,
    ACTIONS(123), 3,
      anon_sym_PERCENT_GT,
      anon_sym_DASH_PERCENT_GT,
      anon_sym__PERCENT_GT,
  [226] = 4,
    ACTIONS(125), 1,
      aux_sym_code_token1,
    ACTIONS(128), 1,
      anon_sym_PERCENT_PERCENT_GT,
    STATE(18), 1,
      aux_sym_code_repeat1,
    ACTIONS(123), 3,
      anon_sym_PERCENT_GT,
      anon_sym_DASH_PERCENT_GT,
      anon_sym_EQ_PERCENT_GT,
  [241] = 4,
    ACTIONS(131), 1,
      aux_sym_code_token1,
    ACTIONS(133), 1,
      anon_sym_PERCENT_PERCENT_GT,
    STATE(18), 1,
      aux_sym_code_repeat1,
    ACTIONS(115), 3,
      anon_sym_PERCENT_GT,
      anon_sym_DASH_PERCENT_GT,
      anon_sym_EQ_PERCENT_GT,
  [256] = 5,
    ACTIONS(135), 1,
      aux_sym_code_token1,
    ACTIONS(137), 1,
      anon_sym_PERCENT_PERCENT_GT,
    ACTIONS(139), 1,
      anon_sym_PERCENT_GT,
    STATE(22), 1,
      aux_sym_code_repeat1,
    STATE(28), 1,
      sym_code,
  [272] = 5,
    ACTIONS(135), 1,
      aux_sym_code_token1,
    ACTIONS(137), 1,
      anon_sym_PERCENT_PERCENT_GT,
    ACTIONS(141), 1,
      anon_sym_PERCENT_GT,
    STATE(22), 1,
      aux_sym_code_repeat1,
    STATE(27), 1,
      sym_code,
  [288] = 4,
    ACTIONS(115), 1,
      anon_sym_PERCENT_GT,
    ACTIONS(143), 1,
      aux_sym_code_token1,
    ACTIONS(145), 1,
      anon_sym_PERCENT_PERCENT_GT,
    STATE(23), 1,
      aux_sym_code_repeat1,
  [301] = 4,
    ACTIONS(123), 1,
      anon_sym_PERCENT_GT,
    ACTIONS(147), 1,
      aux_sym_code_token1,
    ACTIONS(150), 1,
      anon_sym_PERCENT_PERCENT_GT,
    STATE(23), 1,
      aux_sym_code_repeat1,
  [314] = 1,
    ACTIONS(153), 3,
      anon_sym_PERCENT_GT,
      anon_sym_DASH_PERCENT_GT,
      anon_sym__PERCENT_GT,
  [320] = 1,
    ACTIONS(155), 3,
      anon_sym_PERCENT_GT,
      anon_sym_DASH_PERCENT_GT,
      anon_sym_EQ_PERCENT_GT,
  [326] = 1,
    ACTIONS(157), 1,
      ts_builtin_sym_end,
  [330] = 1,
    ACTIONS(159), 1,
      anon_sym_PERCENT_GT,
  [334] = 1,
    ACTIONS(161), 1,
      anon_sym_PERCENT_GT,
};

static const uint32_t ts_small_parse_table_map[] = {
  [SMALL_STATE(6)] = 0,
  [SMALL_STATE(7)] = 20,
  [SMALL_STATE(8)] = 40,
  [SMALL_STATE(9)] = 60,
  [SMALL_STATE(10)] = 80,
  [SMALL_STATE(11)] = 100,
  [SMALL_STATE(12)] = 120,
  [SMALL_STATE(13)] = 140,
  [SMALL_STATE(14)] = 160,
  [SMALL_STATE(15)] = 178,
  [SMALL_STATE(16)] = 196,
  [SMALL_STATE(17)] = 211,
  [SMALL_STATE(18)] = 226,
  [SMALL_STATE(19)] = 241,
  [SMALL_STATE(20)] = 256,
  [SMALL_STATE(21)] = 272,
  [SMALL_STATE(22)] = 288,
  [SMALL_STATE(23)] = 301,
  [SMALL_STATE(24)] = 314,
  [SMALL_STATE(25)] = 320,
  [SMALL_STATE(26)] = 326,
  [SMALL_STATE(27)] = 330,
  [SMALL_STATE(28)] = 334,
};

static const TSParseActionEntry ts_parse_actions[] = {
  [0] = {.entry = {.count = 0, .reusable = false}},
  [1] = {.entry = {.count = 1, .reusable = false}}, RECOVER(),
  [3] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_template, 0, 0, 0),
  [5] = {.entry = {.count = 1, .reusable = false}}, SHIFT(4),
  [7] = {.entry = {.count = 1, .reusable = true}}, SHIFT(4),
  [9] = {.entry = {.count = 1, .reusable = false}}, SHIFT(15),
  [11] = {.entry = {.count = 1, .reusable = true}}, SHIFT(15),
  [13] = {.entry = {.count = 1, .reusable = false}}, SHIFT(14),
  [15] = {.entry = {.count = 1, .reusable = true}}, SHIFT(14),
  [17] = {.entry = {.count = 1, .reusable = true}}, SHIFT(20),
  [19] = {.entry = {.count = 1, .reusable = true}}, SHIFT(21),
  [21] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_template, 1, 0, 0),
  [23] = {.entry = {.count = 1, .reusable = true}}, REDUCE(aux_sym_template_repeat1, 2, 0, 0),
  [25] = {.entry = {.count = 2, .reusable = false}}, REDUCE(aux_sym_template_repeat1, 2, 0, 0), SHIFT_REPEAT(4),
  [28] = {.entry = {.count = 2, .reusable = true}}, REDUCE(aux_sym_template_repeat1, 2, 0, 0), SHIFT_REPEAT(4),
  [31] = {.entry = {.count = 2, .reusable = false}}, REDUCE(aux_sym_template_repeat1, 2, 0, 0), SHIFT_REPEAT(15),
  [34] = {.entry = {.count = 2, .reusable = true}}, REDUCE(aux_sym_template_repeat1, 2, 0, 0), SHIFT_REPEAT(15),
  [37] = {.entry = {.count = 2, .reusable = false}}, REDUCE(aux_sym_template_repeat1, 2, 0, 0), SHIFT_REPEAT(14),
  [40] = {.entry = {.count = 2, .reusable = true}}, REDUCE(aux_sym_template_repeat1, 2, 0, 0), SHIFT_REPEAT(14),
  [43] = {.entry = {.count = 2, .reusable = true}}, REDUCE(aux_sym_template_repeat1, 2, 0, 0), SHIFT_REPEAT(20),
  [46] = {.entry = {.count = 2, .reusable = true}}, REDUCE(aux_sym_template_repeat1, 2, 0, 0), SHIFT_REPEAT(21),
  [49] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_content, 1, 0, 0),
  [51] = {.entry = {.count = 1, .reusable = false}}, SHIFT(5),
  [53] = {.entry = {.count = 1, .reusable = true}}, SHIFT(5),
  [55] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_content, 1, 0, 0),
  [57] = {.entry = {.count = 1, .reusable = true}}, REDUCE(aux_sym_content_repeat1, 2, 0, 0),
  [59] = {.entry = {.count = 2, .reusable = false}}, REDUCE(aux_sym_content_repeat1, 2, 0, 0), SHIFT_REPEAT(5),
  [62] = {.entry = {.count = 2, .reusable = true}}, REDUCE(aux_sym_content_repeat1, 2, 0, 0), SHIFT_REPEAT(5),
  [65] = {.entry = {.count = 1, .reusable = false}}, REDUCE(aux_sym_content_repeat1, 2, 0, 0),
  [67] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_directive, 2, 0, 0),
  [69] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_directive, 2, 0, 0),
  [71] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_output_directive, 2, 0, 0),
  [73] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_output_directive, 2, 0, 0),
  [75] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_graphql_directive, 3, 0, 0),
  [77] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_graphql_directive, 3, 0, 0),
  [79] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_directive, 3, 0, 0),
  [81] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_directive, 3, 0, 0),
  [83] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_comment_directive, 2, 0, 0),
  [85] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_comment_directive, 2, 0, 0),
  [87] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_graphql_directive, 2, 0, 0),
  [89] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_graphql_directive, 2, 0, 0),
  [91] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_output_directive, 3, 0, 0),
  [93] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_output_directive, 3, 0, 0),
  [95] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_comment_directive, 3, 0, 1),
  [97] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_comment_directive, 3, 0, 1),
  [99] = {.entry = {.count = 1, .reusable = false}}, SHIFT(19),
  [101] = {.entry = {.count = 1, .reusable = true}}, SHIFT(19),
  [103] = {.entry = {.count = 1, .reusable = true}}, SHIFT(7),
  [105] = {.entry = {.count = 1, .reusable = false}}, SHIFT(16),
  [107] = {.entry = {.count = 1, .reusable = true}}, SHIFT(16),
  [109] = {.entry = {.count = 1, .reusable = true}}, SHIFT(6),
  [111] = {.entry = {.count = 1, .reusable = false}}, SHIFT(17),
  [113] = {.entry = {.count = 1, .reusable = true}}, SHIFT(17),
  [115] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_code, 1, 0, 0),
  [117] = {.entry = {.count = 2, .reusable = false}}, REDUCE(aux_sym_code_repeat1, 2, 0, 0), SHIFT_REPEAT(17),
  [120] = {.entry = {.count = 2, .reusable = true}}, REDUCE(aux_sym_code_repeat1, 2, 0, 0), SHIFT_REPEAT(17),
  [123] = {.entry = {.count = 1, .reusable = true}}, REDUCE(aux_sym_code_repeat1, 2, 0, 0),
  [125] = {.entry = {.count = 2, .reusable = false}}, REDUCE(aux_sym_code_repeat1, 2, 0, 0), SHIFT_REPEAT(18),
  [128] = {.entry = {.count = 2, .reusable = true}}, REDUCE(aux_sym_code_repeat1, 2, 0, 0), SHIFT_REPEAT(18),
  [131] = {.entry = {.count = 1, .reusable = false}}, SHIFT(18),
  [133] = {.entry = {.count = 1, .reusable = true}}, SHIFT(18),
  [135] = {.entry = {.count = 1, .reusable = false}}, SHIFT(22),
  [137] = {.entry = {.count = 1, .reusable = true}}, SHIFT(22),
  [139] = {.entry = {.count = 1, .reusable = true}}, SHIFT(10),
  [141] = {.entry = {.count = 1, .reusable = true}}, SHIFT(11),
  [143] = {.entry = {.count = 1, .reusable = false}}, SHIFT(23),
  [145] = {.entry = {.count = 1, .reusable = true}}, SHIFT(23),
  [147] = {.entry = {.count = 2, .reusable = false}}, REDUCE(aux_sym_code_repeat1, 2, 0, 0), SHIFT_REPEAT(23),
  [150] = {.entry = {.count = 2, .reusable = true}}, REDUCE(aux_sym_code_repeat1, 2, 0, 0), SHIFT_REPEAT(23),
  [153] = {.entry = {.count = 1, .reusable = true}}, SHIFT(9),
  [155] = {.entry = {.count = 1, .reusable = true}}, SHIFT(12),
  [157] = {.entry = {.count = 1, .reusable = true}},  ACCEPT_INPUT(),
  [159] = {.entry = {.count = 1, .reusable = true}}, SHIFT(8),
  [161] = {.entry = {.count = 1, .reusable = true}}, SHIFT(13),
};

#ifdef __cplusplus
extern "C" {
#endif
#ifdef TREE_SITTER_HIDE_SYMBOLS
#define TS_PUBLIC
#elif defined(_WIN32)
#define TS_PUBLIC __declspec(dllexport)
#else
#define TS_PUBLIC __attribute__((visibility("default")))
#endif

TS_PUBLIC const TSLanguage *tree_sitter_embedded_template(void) {
  static const TSLanguage language = {
    .version = LANGUAGE_VERSION,
    .symbol_count = SYMBOL_COUNT,
    .alias_count = ALIAS_COUNT,
    .token_count = TOKEN_COUNT,
    .external_token_count = EXTERNAL_TOKEN_COUNT,
    .state_count = STATE_COUNT,
    .large_state_count = LARGE_STATE_COUNT,
    .production_id_count = PRODUCTION_ID_COUNT,
    .field_count = FIELD_COUNT,
    .max_alias_sequence_length = MAX_ALIAS_SEQUENCE_LENGTH,
    .parse_table = &ts_parse_table[0][0],
    .small_parse_table = ts_small_parse_table,
    .small_parse_table_map = ts_small_parse_table_map,
    .parse_actions = ts_parse_actions,
    .symbol_names = ts_symbol_names,
    .symbol_metadata = ts_symbol_metadata,
    .public_symbol_map = ts_symbol_map,
    .alias_map = ts_non_terminal_alias_map,
    .alias_sequences = &ts_alias_sequences[0][0],
    .lex_modes = ts_lex_modes,
    .lex_fn = ts_lex,
    .primary_state_ids = ts_primary_state_ids,
  };
  return &language;
}
#ifdef __cplusplus
}
#endif
