#pragma once

uint16_t ascii2xtMapping[] = {
  // 0x
  0, // NUL
  0, // SOH
  0, // STX
  0, // ETX
  0, // EOT
  0, // ENQ
  0, // ACK
  0, // BEL
  0xe, // BS
  0xf, // HT
  0, // LF
  0, // VT
  0, // FF
  0x1c, // CR
  0, // SO
  0, // SI 
  //1x
  0, // DLE
  0, // DC1
  0, // DC2
  0, // DC3
  0, // DC4
  0, // NAK
  0, // SYN
  0, // ETB
  0, // CAN
  0, // EM
  0, // SUB
  0x1, // ESC
  0, // FS
  0, // GS
  0, // RS
  0, // US
  // 2x
  0x39, // SP
  0, // !
  0, // "
  0, // #
  0, // $
  0, // %
  0, // &
  0x28, // '
  0x1a, // (
  0x1b, // )
  0, // *
  0, // +
  0x33, // ,
  0xc, // -
  0x34, // .
  0x35, // /
  // 3x
  0xb, // 0
  0x2, // 1
  0x3, // 2
  0x4, // 3
  0x5, // 4
  0x6, // 5
  0x7, // 6
  0x8, // 7
  0x9, // 8
  0xa, // 9
  0, // :
  0x27, // ;
  0, // <
  0xd, // =
  0, // >
  0, // ?
  // 4x
  0, // @
  0x1e, // A
  0x30, // B
  0x2e, // C
  0x20, // D
  0x12, // E
  0x21, // F
  0x22, // G
  0x23, // H
  0x17, // I
  0x24, // J
  0x25, // K
  0x26, // L
  0x32, // M
  0x31, // N
  0x18, // O
  // 5x
  0x19, // P
  0x10, // Q
  0x13, // R
  0x1f, // S
  0x14, // T
  0x16, // U
  0x2f, // V
  0x11, // W
  0x2d, // X
  0x15, // Y
  0x2c, // Z
  0, // [
  0x2b, // Backslash
  0, // ]
  0, // ^
  0, // _
  // 6x
  0x29, // `
  0x1e, // a
  0x30, // b
  0x2e, // c
  0x20, // d
  0x12, // e
  0x21, // f
  0x22, // g
  0x23, // h
  0x17, // i
  0x24, // j
  0x25, // k
  0x26, // l
  0x32, // m
  0x31, // n
  0x18, // o
  // 7x
  0x19, // p
  0x10, // q
  0x13, // r
  0x1f, // s
  0x14, // t
  0x16, // u
  0x2f, // v
  0x11, // w
  0x2d, // x
  0x15, // y
  0x2c, // z
  0, // {
  0, // |
  0, // }
  0, // ~
  0 // DEL

	// 0x3a,		// USB 39	CapsLock
	// 0x3b,		// USB 3a	F1
	// 0x3c,		// USB 3b	F2
	// 0x3d,		// USB 3c	F3
	// 0x3e,		// USB 3d	F4
	// 0x3f,		// USB 3e	F5
	// 0x40,		// USB 3f	F6
	// 0x41,		// USB 40	F7
	// 0x42,		// USB 41	F8
	// 0x43,		// USB 42	F9
	// 0x44,		// USB 43	F10
	// 0x57,		// USB 44	F11
	// 0x58,		// USB 45	F12
	// 0xe037,		// USB 46	PrintScreen
	// 0x46,		// USB 47	ScrollLock
	// 0xe052,		// USB 49	Insert
	// 0xe047,		// USB 4a	Home
	// 0xe049,		// USB 4b	PageUp
	// 0xe053,		// USB 4c	Delete
	// 0xe04f,		// USB 4d	End
	// 0xe051,		// USB 4e	PageDown
	// 0xe04d,		// USB 4f	ArrowRight
	// 0xe04b,		// USB 50	ArrowLeft
	// 0xe050,		// USB 51	ArrowDown
	// 0xe048,		// USB 52	ArrowUp
	// 0x45,		// USB 53	NumLock
	// 0xe035,		// USB 54	NumpadDivide
	// 0x37,		// USB 55	NumpadMultiply
	// 0x4a,		// USB 56	NumpadSubtract
	// 0x4e,		// USB 57	NumpadAdd
	// 0xe01c,		// USB 58	NumpadEnter
	// 0x4f,		// USB 59	Numpad1
	// 0x50,		// USB 5a	Numpad2
	// 0x51,		// USB 5b	Numpad3
	// 0x4b,		// USB 5c	Numpad4
	// 0x4c,		// USB 5d	Numpad5
	// 0x4d,		// USB 5e	Numpad6
	// 0x47,		// USB 5f	Numpad7
	// 0x48,		// USB 60	Numpad8
	// 0x49,		// USB 61	Numpad9
	// 0x52,		// USB 62	Numpad0
	// 0x53,		// USB 63	NumpadDecimal
	// 0x56,		// USB 64	IntlBackslash
	// 0xe05d,		// USB 65	ContextMenu
	// 0x5b,		// USB 68	F13
	// 0x5c,		// USB 69	F14
	// 0x5d,		// USB 6a	F15
	// 0x63,		// USB 6b	F16
	// 0x64,		// USB 6c	F17
	// 0x65,		// USB 6d	F18
	// 0x66,		// USB 6e	F19
	// 0x67,		// USB 6f	F20
	// 0x68,		// USB 70	F21
	// 0x69,		// USB 71	F22
	// 0x6a,		// USB 72	F23
	// 0x6b,		// USB 73	F24
	// 0xe03b,		// USB 75	Help
	// 0xe008,		// USB 7a	Undo
	// 0xe017,		// USB 7b	Cut
	// 0xe018,		// USB 7c	Copy
	// 0xe00a,		// USB 7d	Paste
	// 0xe020,		// USB 7f	VolumeMute
	// 0xe030,		// USB 80	VolumeUp
	// 0xe02e,		// USB 81	VolumeDown
	// 0x7d,		// USB 89	IntlYen
	// 0x1d,		// USB e0	ControlLeft
	// 0x2a,		// USB e1	ShiftLeft
	// 0x38,		// USB e2	AltLeft
	// 0xe05b,		// USB e3	OSLeft
	// 0xe01d,		// USB e4	ControlRight
	// 0x36,		// USB e5	ShiftRight
	// 0xe038,		// USB e6	AltRight
	// 0xe05c,		// USB e7	OSRight
};
