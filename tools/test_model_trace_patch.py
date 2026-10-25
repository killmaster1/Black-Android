#!/usr/bin/env python3
"""Confere geração CMake e rejeição de SDK sem o ponto de instrumentação."""
from pathlib import Path
import subprocess
import sys
import tempfile
raiz=Path(__file__).resolve().parent.parent
sdk=Path(sys.argv[1]).resolve();cmake=sys.argv[2]
with tempfile.TemporaryDirectory() as t:
 p=Path(t);src=p/'src';src.mkdir()
 def projeto(onde):
  (src/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.22)
project(TesteModelo NONE)
set(SDK "{onde.as_posix()}")
set(RUNTIME_SOURCES "${{SDK}}/runtime/kernel/files.cpp")
set(BLACK_TRACE_MODELS ON)
include("{(raiz/'cmake/ModelTrace.cmake').as_posix()}")
list(LENGTH RUNTIME_SOURCES n)
if(NOT n EQUAL 1)
 message(FATAL_ERROR "Fonte duplicada")
endif()
''')
 projeto(sdk)
 r=subprocess.run([cmake,'-S',str(src),'-B',str(p/'build')],capture_output=True,text=True)
 assert r.returncode==0,r.stdout+r.stderr
 patch=(p/'build/model_trace_files.cpp').read_text()
 assert patch.count('model_trace::Read(')==1 and 'model_trace::Wanted(f->xbox)' in patch
 assert 'model_trace::Read(' not in (sdk/'runtime/kernel/files.cpp').read_text()
 falso=p/'sdk';(falso/'runtime/kernel').mkdir(parents=True)
 (falso/'runtime/kernel/files.cpp').write_text('// Layout desconhecido, sem Transfer\n')
 projeto(falso)
 r=subprocess.run([cmake,'-S',str(src),'-B',str(p/'invalido')],capture_output=True,text=True)
 assert r.returncode!=0 and 'SDK incompatível' in r.stderr
print('CMake: cópia instrumentada única, SDK original intacto e layout desconhecido rejeitado: OK')
