#ifndef MARKDOWNACTIONS_H
#define MARKDOWNACTIONS_H

#include <string>

enum MdActionId {
    MdActBold,
    MdActItalic,
    MdActCode,
    MdActStrike,
    MdActHeading,
    MdActBullet,
    MdActNumber,
    MdActTask,
    MdActQuote,
    MdActDone,
    MdActIndent,
    MdActOutdent,
    MdActLink,
    MdActRule,
    MdActDate,
    MdActTime,
    MdActEnter
};

struct MdActionCtx {
    std::string dateText;
    std::string timeText;
    int indentWidth;

    MdActionCtx() : indentWidth(2) {}
};

struct MdActionResult {
    std::string text;
    int selStart;
    int selEnd;
};

MdActionResult mdApplyAction(MdActionId id, const std::string &text, int selStart, int selEnd, const MdActionCtx &ctx);

#endif
