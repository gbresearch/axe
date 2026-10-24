-- pandoc Lua filter: the <br> line breaks in the table cells of AXE-Reference.md are raw HTML, which only
-- the HTML output keeps; this turns them into line breaks for the other formats, e.g. Word (docs/README.md)
function RawInline(el)
    if el.format == 'html' and el.text:match('^<br%s*/?>$') then
        return pandoc.LineBreak()
    end
end
