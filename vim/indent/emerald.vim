if exists('b:did_indent_emerald')
  finish
endif
let b:did_indent_emerald = 1

" Simple indentation: indent after lines ending with ':'
function! s:GetIndent()
  let lnum = prevnonblank(v:lnum - 1)
  if lnum == 0
    return 0
  endif
  let line = getline(lnum)
  if line =~ ':$'
    return indent(lnum) + &shiftwidth
  endif
  return indent(lnum)
endfunction

setlocal indentexpr=s:GetIndent()
