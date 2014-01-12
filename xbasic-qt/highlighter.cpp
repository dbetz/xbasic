/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

#include "highlighter.h"

//! [0]
Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    keywordFormat.setForeground(Qt::blue);
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;
    keywordPatterns << "\\b[Ii][Nn][Cc][Ll][Uu][Dd][Ee]\\b"
                    << "\\b[Oo][Pp][Tt][Ii][Oo][Nn]\\b"
                    << "\\b[Ss][Tt][Aa][Cc][Kk][Ss]][Ii][Zz][Ee]\\b"
                    << "\\b[Aa][Ss]\\b" << "\\b[Bb][Yy][Tt][Ee]\\b"
                    << "\\b[Ii][Nn][Tt][Ee][Gg][Ee][Rr]\\b"
                    << "\\b[Ii][Nn]\\b"
                    << "\\b[Rr][Ee][Mm]\\b"
                    << "\\b[Dd][Ee][Ff]\\b" << "\\b[Dd][Ii][Mm]\\b"
                    << "\\b[Ee][Nn][Dd]\\b"
                    << "\\b[Ii][Ff]\\b" << "\\b[Tt][Hh][Ee][Nn]\\b"
                    << "\\b[Ee][Ll][Ss][Ee]\\b" << "\\b[Nn][Oo][Tt]\\b"
                    << "\\b[Ss][Ee][Ll][Ee][Cc][Tt]\\b" << "\\b[Cc][Aa][Ss][Ee]\\b"
                    << "\\b[Tt][Oo]\\b" << "\\b[Gg][Oo][Tt][Oo]\\b"
                    << "\\b[Ss][Tt][Oo][Pp]\\b" << "\\b[Ee][Nn][Dd]\\b"
                    << "\\b[Ff][Oo][Rr]\\b" << "\\b[Nn][Ee][Xx][Tt]\\b"
                    << "\\b[Dd][Oo]\\b" << "\\b[Ww][Hh][Ii][Ll][Ee]\\b"
                    << "\\b[Ll][Oo][Oo][Pp]\\b" << "\\b[Uu][Nn][Tt][Ii][Ll]\\b"
                    << "\\b[Rr][Ee][Tt][Uu][Rr][Nn]\\b"
                    << "\\b[Pp][Rr][Ii][Nn][Tt]\\b"
                    << "\\b[Ii][Nn][Pp][Uu][Tt]\\b"
                    << "\\b[Pp][Aa][Rr]\\b" << "\\b[Cc][Nn][Tt]\\b"
                    << "\\b[Ii][Nn][Aa]\\b" << "\\b[Ii][Nn][Bb]\\b"
                    << "\\b[Oo][Uu][Tt][Aa]\\b" << "\\b[Oo][Uu][Tt][Bb]\\b"
                    << "\\b[Dd][Ii][Rr][Aa]\\b" << "\\b[Dd][Ii][Rr][Bb]\\b"
                    << "\\b[Cc][Tt][Rr][Aa]\\b" << "\\b[Cc][Tt][Rr][Bb]\\b"
                    << "\\b[Ff][Rr][Qq][Aa]\\b" << "\\b[Ff][Rr][Qq][Bb]\\b"
                    << "\\b[Pp][Hh][Ss][Aa]\\b" << "\\b[Pp][Hh][Ss][Bb]\\b"
                    << "\\b[Vv][Cc][Ff][Gg]\\b" << "\\b[Vv][Ss][Cc][Ll]\\b"
                    << "\\b[Cc][Ll][Kk][Ff][Rr][Ee][Qq]\\b"
                    << "={2,}" << "-{2,}" << "_{2,}" << "\\{2,}"
                    ;
    foreach (const QString &pattern, keywordPatterns) {
        rule.pattern = QRegExp(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
//! [0] //! [1]
    }
//! [1]

//! [2]
    classFormat.setFontWeight(QFont::Bold);
    classFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp("\\b[Rr][Ee][Mm][^\n]*");
    rule.format = classFormat;
    highlightingRules.append(rule);
//! [2]

//! [3]
    singleLineCommentFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp("//[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    multiLineCommentFormat.setForeground(Qt::darkGreen);
//! [3]

//! [4]
    quotationFormat.setForeground(Qt::red);
    rule.pattern = QRegExp("\".*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);
//! [4]

//! [5]
    functionFormat.setFontItalic(true);
    functionFormat.setForeground(Qt::blue);
    rule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);
//! [5]

//! [6]
    commentStartExpression = QRegExp("/\\*");
    commentEndExpression = QRegExp("\\*/");
}
//! [6]

//! [7]
void Highlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
//! [7] //! [8]
    setCurrentBlockState(0);
//! [8]

//! [9]
    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = commentStartExpression.indexIn(text);

//! [9] //! [10]
    while (startIndex >= 0) {
//! [10] //! [11]
        int endIndex = commentEndExpression.indexIn(text, startIndex);
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex
                            + commentEndExpression.matchedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = commentStartExpression.indexIn(text, startIndex + commentLength);
    }
}
//! [11]
