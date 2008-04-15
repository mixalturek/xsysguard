" Vim filetype detection file
" Language:	xsysguard
" Last Change:	2008-04-14
" Author:	Sascha Wessel <sawe@users.sf.net>
" URL:		http://xsysguard.sf.net
" License:	GPL

" ~/.vim/ftdetect/xsysguard_filetype.vim

au BufNewFile,BufRead ~/.xsysguard/configs/* setf xsysguard
au BufNewFile,BufRead */share/xsysguard/configs/* setf xsysguard

