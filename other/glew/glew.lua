Glew = {
	basepath = PathDir(ModuleFilename()),
	
	OptFind = function (name, required)	
		local check = function(option, settings)
			option.value = false
			option.use_glewconfig = false
			option.use_win32lib = false
			option.lib_path = nil
			
			if ExecuteSilent("glew-config") > 0 and ExecuteSilent("glew-config --cflags") == 0 then
				option.value = true
				option.use_glewconfig = true
			end
				
			if platform == "win32" then
				option.value = true
				option.use_win32lib = true
			end
		end
		
		local apply = function(option, settings)
			-- include path
			settings.cc.includes:Add(Glew.basepath .. "/include")
			
			if option.use_glewconfig == true then
				settings.cc.flags:Add("`glew-config --cflags`")
				settings.link.flags:Add("`glew-config --libs`")
				
			elseif option.use_win32lib == true then
				settings.link.libpath:Add(Glew.basepath .. "/lib")
				settings.link.libs:Add("glew32")
			end
		end
		
		local save = function(option, output)
			output:option(option, "value")
			output:option(option, "use_glewconfig")
			output:option(option, "use_win32lib")
		end
		
		local display = function(option)
			if option.value == true then
				if option.use_glewconfig == true then return "using glew-config" end
				if option.use_win32lib == true then return "using supplied win32 libraries" end
				return "using unknown method"
			else
				if option.required then
					return "not found (required)"
				else
					return "not found (optional)"
				end
			end
		end
		
		local o = MakeOption(name, 0, check, save, display)
		o.Apply = apply
		o.include_path = nil
		o.lib_path = nil
		o.required = required
		return o
	end
}
