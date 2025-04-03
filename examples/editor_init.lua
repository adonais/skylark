require("eu")

function skylark_init()
    local handle, pid = eu.create_process("E:\\soft\\mpv\\mpv.exe", "\"E:\\movies\\峡谷\\The Gorge 2025 1080p.mkv\"", 2)
    if (handle ~= nil) then
        print(string.format("we create proces, pid = %u\n", pid))
        eu.ffi.C.CloseHandle(eu.ffi.cast("void *", handle))
    end
end

function main()
    local ret = eu.lib.register_event(SKYLARK_INIT, "skylark_init")
    if (ret ~= 1) then
        print("Loader: register skylark_init envent failed")
    end
end
