eu = {}

require("eu_core")
require("eu_sci")

eu.ffi = eu_core.ffi
eu.api = eu_core.euapi
eu.lib = eu_core.eulib

function eu.msgbox(text, cap, mb)
    local content = eu.api.eu_utf8_utf16(text, nil)
    local tip = eu.api.eu_utf8_utf16(cap, nil)
    eu.ffi.gc(content,(function(self)
        if content ~= nil then eu.ffi.C.free(content) end
    end))
    eu.ffi.gc(tip,(function(self)
        if tip ~= nil then eu.ffi.C.free(tip) end
    end))
    if (content ~= nil and tip ~= nil) then
        return eu.api.eu_msgbox(eu.api.eu_module_hwnd(), content, tip, mb)
    end
    return 0
end

function eu.window_title()
    local title = nil
    local buf = eu.ffi.C.malloc(1024 * 2)
    eu.ffi.gc(buf,(function(self)
        if buf ~= nil then eu.ffi.C.free(buf) end
    end))
    if (buf ~= nil and eu.ffi.C.GetWindowTextW(eu.api.eu_module_hwnd(), buf, 1024) > 0) then
        title = eu.api.eu_utf16_utf8(buf, nil)
    end
    eu.ffi.gc(title,(function(self)
        if title ~= nil then eu.ffi.C.free(title) end
    end))
    return eu.ffi.string(title, eu.ffi.C.strlen(title))
end

function eu.create_process(path, param, flags)
    local handle = nil
    local pid = nil
    local buf = eu.api.eu_utf8_utf16(path, nil)
    if (buf ~= nil) then
        eu.ffi.gc(buf,(function(self)
            eu.ffi.C.free(buf)
        end))
        local parg = eu.api.eu_utf8_utf16(param, nil)
        if (parg ~= nil) then
            eu.ffi.gc(parg,(function(self)
                eu.ffi.C.free(parg)
            end))
        end
        local p = eu.ffi.cast("void *", eu.ffi.new("uint32_t[1]", {}))
        handle = eu.api.eu_new_process(buf, parg, nil, flags, p)
        if (handle ~= nil) then
            pid = eu.api.eu_value(p)
        end
    end
    return handle,pid
end

function eu.win10_or_later()
    local ret = true
    if (eu.api.eu_win10_or_later() > 0xFFFFFFFF) then
        ret = false
    end
    return ret
end

function eu.process_id()
    return eu.ffi.C.SendMessageW(eu.api.eu_module_hwnd(), WM_PROCESS_ID, 0, 0)
end

function eu.close_tab()
    -- WM_COMMAND == 0x0111
    return eu.ffi.C.SendMessageW(eu.api.eu_module_hwnd(), 0x0111, IDM_FILE_CLOSE, 0)
end

function eu.max_position(index)
    if (index == nil) then index = -1 end
    return eu.api.eu_end_positon(index)
end

function eu.line_start_positon(index, line)
    if (index == nil) then index = -1 end
    return eu.api.eu_line_start_positon(index, line)
end

function eu.line_end_positon(index, line)
    if (index == nil) then index = -1 end
    return eu.api.eu_line_end_positon(index, line)
end

function eu.file_size(index)
    if (index == nil) then index = -1 end
    return eu.api.eu_file_size(index)
end

function eu.file_path(index)
    if (index == nil) then index = -1 end
    local path = eu.api.eu_file_path(index)
    eu.ffi.gc(path,(function(self)
        if path ~= nil then eu.ffi.C.free(path) end
    end))
    return eu.ffi.string(path, eu.ffi.C.strlen(path))
end

function eu.file_name(index)
    if (index == nil) then index = -1 end
    local name = eu.api.eu_file_name(index)
    eu.ffi.gc(name,(function(self)
        if name ~= nil then eu.ffi.C.free(name) end
    end))
    return eu.ffi.string(name, eu.ffi.C.strlen(name))
end

function eu.line_total(index)
    if (index == nil) then index = -1 end
    return eu.api.eu_sci_call(index, SCI_GETLINECOUNT, 0, 0)
end

function eu.begin_action(index)
    if (index == nil) then index = -1 end
    eu.api.eu_sci_call(index, SCI_BEGINUNDOACTION, 0, 0)
end

function eu.end_action(index)
    if (index == nil) then index = -1 end
    eu.api.eu_sci_call(index, SCI_ENDUNDOACTION, 0, 0)
end

function eu.get_text(index)
    if (index == nil) then index = -1 end
    local str = eu.api.eu_strdup_content(index)
    eu.ffi.gc(str,(function(self)
        if str ~= nil then eu.ffi.C.free(str) end
    end))
    return eu.ffi.string(str, eu.ffi.C.strlen(str))
end

function eu.clear_text(index)
    if (index == nil) then index = -1 end
    eu.api.eu_sci_call(index, SCI_CLEARALL, 0, 0)
end

function eu.set_text(text, index)
    if (index == nil) then index = -1 end
    eu.api.eu_sci_call(index, SCI_ADDTEXT, #text, eu.ffi.cast("const intptr_t", text))
end

function eu.get_line(index, line)
    if (index == nil) then index = -1 end
    local buffer = eu.api.eu_strdup_line(index, line)
    eu.ffi.gc(buffer,(function(self)
        if buffer ~= nil then eu.ffi.C.free(buffer) end
    end))
    return eu.ffi.string(buffer, eu.ffi.C.strlen(buffer))
end

function eu.cureent_line_number(index)
    if (index == nil) then index = -1 end
    local pos = eu.api.eu_sci_call(index, SCI_GETCURRENTPOS, 0, 0)
    return eu.api.eu_sci_call(index, SCI_LINEFROMPOSITION, pos, 0)
end

function eu.get_selection(index)
    if (index == nil) then index = -1 end
    local buf = eu.api.eu_strdup_select(index)
    eu.ffi.gc(buf,(function(self)
        if buf ~= nil then eu.ffi.C.free(buf) end
    end))
    return eu.ffi.string(buf, eu.ffi.C.strlen(buf))
end

function eu.replace_selection(text, index)
    if (index == nil) then index = -1 end
    eu.api.eu_sci_call(index, SCI_REPLACESEL, 0, eu.ffi.cast("const intptr_t", text))
end

function eu.set_selection(pos1, pos2, index)
    if (index == nil) then index = -1 end
    eu.api.eu_sci_call(index, SCI_SETSEL, pos1, pos2)
end

function eu.get_position(index)
    if (index == nil) then index = -1 end
    eu.api.eu_sci_call(index, SCI_GETCURRENTPOS, 0, 0)
end

function eu.current_word(index)
    if (index == nil) then index = -1 end
    local txt = nil
    local pos = eu.api.eu_sci_call(index, SCI_GETCURRENTPOS, 0, 0)
    if (pos ~= nil and pos >= 0) then
        local s1 = eu.api.eu_sci_call(index, SCI_WORDSTARTPOSITION, pos, 1)
        local s2 = eu.api.eu_sci_call(index, SCI_WORDENDPOSITION, pos, 1)
        if (s1 ~= nil and s2 ~= nil) then
            local buf = eu.api.eu_strdup_range(index, s1, s2)
            eu.ffi.gc(buf,(function(self)
                if buf ~= nil then eu.ffi.C.free(buf) end
            end))
            if (buf ~= nil) then
                txt = eu.ffi.string(buf, eu.ffi.C.strlen(buf))
            end
        end
    end
    return txt
end

function eu.sci_cmd(p, m, w, l)
    return eu.api.eu_sci_cmd(p, m, w, l)
end

function eu.close_file(index, mode)
    if (index == nil) then index = -1 end
    if (mode == nil) then mode = 1 end
    return eu.api.eu_file_close(index, mode)
end

function eu.open_file(path)
    local wpath = eu.api.eu_utf8_utf16(path, nil)
    eu.ffi.gc(wpath,(function(self)
        if wpath ~= nil then eu.ffi.C.free(wpath) end
    end))
    return eu.api.eu_file_open(wpath)
end

function eu.save_file(index)
    if (index == nil) then index = -1 end
    return eu.api.eu_command_save(index)
end

return eu
