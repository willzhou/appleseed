
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010 Francois Beaune
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Interface header.
#include "preprocessor.h"

// appleseed.foundation headers.
#include "foundation/core/exceptions/exception.h"
#include "foundation/utility/containers/dictionary.h"
#include "foundation/utility/foreach.h"
#include "foundation/utility/string.h"
#include "foundation/utility/test.h"

// Standard headers.
#include <cassert>
#include <string>
#include <vector>

using namespace std;

namespace foundation
{

//
// Preprocessor class implementation.
//

namespace
{
    bool is_directive(const string& line)
    {
        const string::size_type i = line.find_first_not_of(Blanks);

        return i != string::npos && line[i] == '#';
    }

    void split_line(const string& line, string& keyword, string& arguments)
    {
        string::size_type cursor = line.find_first_not_of(Blanks);

        const string::size_type keyword_begin = cursor;
        cursor = line.find_first_of(Blanks, cursor);

        keyword =
            cursor == string::npos
                ? line.substr(keyword_begin)
                : line.substr(keyword_begin, cursor - keyword_begin);

        arguments.clear();

        if (cursor == string::npos)
            return;

        cursor = line.find_first_not_of(Blanks, cursor);

        if (cursor == string::npos)
            return;

        const string::size_type arguments_begin = cursor;
        cursor = line.find_last_not_of(Blanks);

        arguments = line.substr(arguments_begin, cursor - arguments_begin + 1);
    }

    void split_directive(const string& line, string& keyword, string& arguments)
    {
        const string::size_type i = line.find_first_not_of(Blanks);

        assert(i != string::npos);
        assert(line[i] == '#');

        split_line(line.substr(i + 1), keyword, arguments);
    }

    TEST_SUITE(Foundation_Utility_Preprocessor_Impl)
    {
        TEST_CASE(SplitDirective_GivenKeyword_ReturnsKeyword)
        {
            string keyword, arguments;
            split_directive("#keyword", keyword, arguments);

            EXPECT_EQ("keyword", keyword);
        }

        TEST_CASE(SplitDirective_GivenSpacesBeforeHashCharacter_ReturnsKeyword)
        {
            string keyword, arguments;
            split_directive("   #keyword", keyword, arguments);

            EXPECT_EQ("keyword", keyword);
        }

        TEST_CASE(SplitDirective_GivenSpacesAfterHashCharacter_ReturnsKeyword)
        {
            string keyword, arguments;
            split_directive("#   keyword", keyword, arguments);

            EXPECT_EQ("keyword", keyword);
        }

        TEST_CASE(SplitDirective_GivenSpacesBeforeAndAfterHashCharacter_ReturnsKeyword)
        {
            string keyword, arguments;
            split_directive("   #   keyword", keyword, arguments);

            EXPECT_EQ("keyword", keyword);
        }

        TEST_CASE(SplitDirective_GivenSpacesAfterKeyword_ReturnsKeyword)
        {
            string keyword, arguments;
            split_directive("#keyword   ", keyword, arguments);

            EXPECT_EQ("keyword", keyword);
        }

        TEST_CASE(SplitDirective_GivenNoArgument_ReturnsEmptyArguments)
        {
            string keyword, arguments;
            split_directive("#keyword", keyword, arguments);

            EXPECT_EQ("", arguments);
        }

        TEST_CASE(SplitDirective_GivenArguments_ReturnsArguments)
        {
            string keyword, arguments;
            split_directive("#keyword arg1 arg2", keyword, arguments);

            EXPECT_EQ("arg1 arg2", arguments);
        }

        TEST_CASE(SplitDirective_GivenSpacesAfterArguments_ReturnsArguments)
        {
            string keyword, arguments;
            split_directive("#keyword arg1 arg2   ", keyword, arguments);

            EXPECT_EQ("arg1 arg2", arguments);
        }
    }
}

struct Preprocessor::Impl
{
    struct ExceptionParseError
      : public Exception
    {
        const size_t m_line_number;

        ExceptionParseError(const string& message, const size_t line_number)
          : Exception(message.c_str())
          , m_line_number(line_number)
        {
        }
    };

    bool                    m_succeeded;
    string                  m_error_message;
    size_t                  m_error_location;

    StringDictionary        m_symbols;

    vector<string>          m_input_lines;
    size_t                  m_current_input_line;

    string                  m_result;
    size_t                  m_current_output_line;

    void process_text(const string& text)
    {
        assert(m_result.empty());

        m_succeeded = true;
        m_error_location = 0;
        m_current_input_line = 0;
        m_current_output_line = 0;

        split(text, "\n", m_input_lines);

        parse();

        if (!text.empty() && text[text.size() - 1] == '\n')
            m_result += '\n';
    }

    void parse()
    {
        while (!is_end_of_input_text())
        {
            const string& line = get_next_input_line();

            if (is_directive(line))
            {
                string keyword, arguments;
                split_directive(line, keyword, arguments);
                parse_directive(keyword, arguments);
            }
            else
            {
                process_line(line);
            }
        }
    }

    void parse_directive(const string& keyword, const string& arguments)
    {
        if (keyword == "define")
        {
            parse_define_directive(arguments);
        }
        else if (keyword == "ifdef")
        {
            parse_ifdef_directive(arguments);
        }
        else
        {
            parse_error(string("Unknown directive: #") + keyword);
        }
    }

    void parse_define_directive(const string& arguments)
    {
        string symbol, value;
        split_line(arguments, symbol, value);

        value = perform_symbol_substitutions(value);

        m_symbols.insert(symbol, value);
    }

    void parse_ifdef_directive(const string& arguments)
    {
        const bool condition_value = evaluate_condition(arguments);

        while (true)
        {
            if (is_end_of_input_text())
            {
                parse_error("Expected directive: #endif");
                break;
            }

            const string& line = get_next_input_line();

            if (is_directive(line))
            {
                string keyword, arguments;
                split_directive(line, keyword, arguments);

                if (keyword == "endif")
                    break;
                else parse_directive(keyword, arguments);
            }
            else
            {
                if (condition_value)
                    process_line(line);
            }
        }
    }

    bool evaluate_condition(const string& condition) const
    {
        return m_symbols.exist(condition);
    }

    void process_line(const string& line)
    {
        emit_line(perform_symbol_substitutions(line));
    }

    string perform_symbol_substitutions(const string& line) const
    {
        string s = line;

        for (const_each<StringDictionary> i = m_symbols; i; ++i)
            s = replace(s, i->name(), i->value());

        return s;
    }

    void parse_error(const string& message)
    {
        throw ExceptionParseError(message, m_current_input_line);
    }

    bool is_end_of_input_text() const
    {
        return m_current_input_line == m_input_lines.size();
    }

    const string& get_next_input_line()
    {
        assert(m_current_input_line < m_input_lines.size());

        return m_input_lines[m_current_input_line++];
    }

    void emit_line(const string& line)
    {
        if (m_current_output_line > 0)
            m_result += '\n';

        m_result += line;

        ++m_current_output_line;
    }
};

Preprocessor::Preprocessor()
  : impl(new Impl())
{
}

void Preprocessor::define_symbol(const char* name)
{
    assert(name);

    impl->m_symbols.insert(name, "");
}

void Preprocessor::define_symbol(const char* name, const char* value)
{
    assert(name);
    assert(value);

    impl->m_symbols.insert(name, value);
}

void Preprocessor::process(const char* text)
{
    assert(text);

    try
    {
        impl->process_text(text);
    }
    catch (const Impl::ExceptionParseError& e)
    {
        impl->m_succeeded = false;
        impl->m_error_message = e.what();
        impl->m_error_location = e.m_line_number;
    }
}

const char* Preprocessor::get_processed_text() const
{
    return impl->m_result.c_str();
}

bool Preprocessor::succeeded() const
{
    return impl->m_succeeded;
}

bool Preprocessor::failed() const
{
    return !succeeded();
}

const char* Preprocessor::get_error_message() const
{
    return failed() ? impl->m_error_message.c_str() : 0;
}

size_t Preprocessor::get_error_location() const
{
    return failed() ? impl->m_error_location : ~size_t(0);
}

}   // namespace foundation