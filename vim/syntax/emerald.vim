if exists("b:current_syntax")
  finish
endif

syntax keyword emeraldKeyword def for while return if else match use import in as
syntax match emeraldComment "//.*$"
syntax match emeraldComment "#.*$"
syntax region emeraldString start=+\"+ skip=+\\\"+ end=+\"+

hi def link emeraldKeyword Keyword
hi def link emeraldComment Comment
hi def link emeraldString String

let b:current_syntax = "emerald"
