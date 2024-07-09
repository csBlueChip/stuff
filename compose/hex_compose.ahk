;-------------------------------------------------------------------------------
; Hex compose key - For Windows AutoHotKey v1
;
; The compose key is : {pause}
; Then type two hex digits (¿higits?)
; This will be translated to decimal, and typed in using alt+numpad
;
pause::
	; timeout for subsequent keystrokes (in seconds) eg. "tmo = T1.75"
	; leave completely blank for "no timeout"        eg. "tmo ="
	tmo = T2.0

	; get Hi hex nybble
	Input, keyH, ML1%tmo%, {pause}
	
	; Double tap {pause} - do not compose
	if (ErrorLevel = "EndKey:Pause") {
		Send {pause}
		Return
	}
	
	hi := (InStr("0123456789abcdef", keyH) -1) *0x10

	; if we got a valid hi nybble
	if (hi >= 0) {
		; get lo hex nybble
		Input, keyL, ML1%tmo%
		lo := (InStr("0123456789abcdef", keyL) -1)

		; if we got a valid lo nybble
		if (lo >= 0) {
			; add the two nybbles together
			val := hi + lo

			; 0 is a special case
			if (val == 0) {
				Send ^@

			; any value {0x01..0xFF} {1..255}
			} else {
				; hundreds, then tens, then units
				col = 100
				; do not zero-prefix decimal numbers
				go  = 0

				; capslock must be OFF for ALT codes to work
				caps := GetKeyState("CapsLock", "T") ? "On" : "Off"
				SetCapsLockState, Off
				Send {LAlt Down}

				while (col > 0) {
					dgt := Floor(val / col)
					if (go == 1 || dgt > 0) {
						go = 1
						Switch dgt {
							case  0  : Send {Numpad0}
							case  1  : Send {Numpad1}
							case  2  : Send {Numpad2}
							case  3  : Send {Numpad3}
							case  4  : Send {Numpad4}
							case  5  : Send {Numpad5}
							case  6  : Send {Numpad6}
							case  7  : Send {Numpad7}
							case  8  : Send {Numpad8}
							case  9  : Send {Numpad9}
						}
					}
					val := val - (dgt * col)
					col := Floor(col / 10)
				}

				; finish ALT sequence, and restore the capslock state
				Send {LAlt Up}
				SetCapsLockState, %caps%
			}

		; lo nybble was invalid - do not compose
		} else {
			Send {pause}%keyH%%keyL%
		}

	; hi nybble was invalid - do not compose
	} else {
		Send {pause}%keyH%
	}
