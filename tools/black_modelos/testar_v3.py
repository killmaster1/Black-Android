#!/usr/bin/env python3
"""Regressões binárias da V3; não substituem teste no Black-Android."""
from pathlib import Path
import struct
import tempfile
import unittest
import black_obj
import black_obj_v3 as v3


class TestV3(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.original=Path(__import__('sys').argv[1]);cls.dados=cls.original.read_bytes()
        cls.ms,cls.usados,cls.doadores,_,_=v3.examinar(cls.dados)
        cls.exemplo=Path(__file__).parent/'exemplos/cubo_v3.obj'

    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.p=Path(self.tmp.name)

    def executar(self,nome='novo',obj=None):
        return v3.substituir(self.original,obj or self.exemplo,self.p/f'{nome}.db',self.p/f'{nome}.json',True)

    def test_integridade_determinismo_releitura_e_contagens(self):
        a=self.executar();b=self.executar('outro');raw=(self.p/'novo.db').read_bytes()
        self.assertEqual(a,b);self.assertEqual(raw,(self.p/'outro.db').read_bytes())
        self.assertEqual(self.original.read_bytes(),self.dados)
        self.assertEqual((a['vertices_utilizados'],a['triangulos'],a['indices_utilizados']),(24,12,58))
        ms=black_obj.detectar(raw);self.assertEqual(len(ms),3);self.assertEqual(len(ms[1]['faces']),12)
        self.assertEqual(ms[0],self.ms[0]);self.assertEqual(ms[2],self.ms[2]);self.assertEqual(len(raw),len(self.dados))
        self.assertTrue(all(i in self.usados[1] for f in ms[1]['faces'] for i in f))

    def test_atributos_uv_normais_maos_e_intervalos(self):
        r=self.executar();raw=(self.p/'novo.db').read_bytes();pontos,faces,_=v3.ler_obj(self.exemplo)
        slots=sorted(self.usados[1]);permitidos=set();m=self.ms[1]
        atributos=bytes.fromhex(r['atributos_12_23_hex'])
        for g in (1,2):
            for k,p in enumerate(pontos):
                o=self.ms[g]['inicio']+slots[k]*36;permitidos.update(range(o,o+36))
                self.assertEqual(raw[o+12:o+24],atributos)
                self.assertEqual(raw[o+28:o+36],struct.pack('<2f',*p[1]))
                self.assertAlmostEqual(sum(x*x for x in v3.decodificar_normal(black_obj.u32(raw,o+24))),1.,places=3)
            for i in self.usados[2]:
                o=self.ms[g]['inicio']+i*36;self.assertEqual(raw[o:o+36],self.dados[o:o+36])
        a,b=self.ms[1:]
        self.assertEqual(raw[a['inicio']:a['inicio']+5152*36],raw[b['inicio']:b['inicio']+5152*36])
        ib=m['limites']+black_obj.u32(self.dados,m['limites']+40)
        permitidos.update(range(ib,ib+r['indices_utilizados']*2))
        for off,n in [(0,12),(16,12),(36,4)]:permitidos.update(range(m['limites']+off,m['limites']+off+n))
        self.assertTrue(all(i in permitidos for i,(a,b) in enumerate(zip(self.dados,raw)) if a!=b))
        self.assertEqual(len(self.usados[1]&self.usados[2]),0)

    def test_topologia_e_orientacao(self):
        self.executar();raw=(self.p/'novo.db').read_bytes();_,faces,_=v3.ler_obj(self.exemplo)
        slots=sorted(self.usados[1]);self.assertEqual(black_obj.detectar(raw)[1]['faces'],[[slots[i] for i in f] for f in faces])

    def test_recusa_capacidade_sem_gravar(self):
        texto=self.exemplo.read_text();f='\n'.join(x for x in texto.splitlines() if x.startswith('f '))+'\n'
        obj=self.p/'grande.obj';obj.write_text(texto+f*200)
        with self.assertRaisesRegex(ValueError,'Capacidade'):self.executar(obj=obj)
        self.assertFalse((self.p/'novo.db').exists());self.assertFalse((self.p/'novo.json').exists())

    def test_original_incompativel_e_sobrescrita(self):
        alterado=self.p/'original.db';b=bytearray(self.dados);b[0]^=1;alterado.write_bytes(b)
        with self.assertRaisesRegex(ValueError,'homologado'):v3.substituir(alterado,self.exemplo,self.p/'x.db',self.p/'x.json',True)
        self.executar()
        with self.assertRaisesRegex(ValueError,'existe'):self.executar()
        with self.assertRaisesRegex(ValueError,'distintos'):v3.substituir(self.original,self.exemplo,self.p/'a',self.p/'a',True)

    def test_triangulacao_concava_indices_negativos_uv_ausente(self):
        obj=self.p/'concavo.obj';obj.write_text('v 0 0 0\nv 2 0 0\nv 2 2 0\nv 1 1 0\nv 0 2 0\nf -5 -4 -3 -2 -1\n')
        pontos,faces,sem_uv=v3.ler_obj(obj);self.assertEqual(len(faces),3);self.assertTrue(sem_uv)
        area=0
        for f in faces:
            a,b,c=[pontos[i][0] for i in f]
            area+=((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]))/2
        self.assertAlmostEqual(area,3)

    def test_obj_invalido_e_nao_plano(self):
        for texto in ['v nan 0 0\nf 1 1 1\n', 'v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 9\n',
                      'v 0 0 0\nv 1 0 0\nv 1 1 1\nv 0 1 0\nf 1 2 3 4\n',
                      'v 0 0 0\nv 1 1 0\nv 0 1 0\nv 1 0 0\nf 1 2 3 4\n']:
            obj=self.p/'invalido.obj';obj.write_text(texto)
            with self.assertRaises(ValueError):v3.ler_obj(obj)

    def test_verificador_de_log(self):
        from verificar_log_v3 import verificar
        import zlib
        crc=f'{zlib.crc32(self.dados):08x}'
        log=('MODELO fonte_selecionada=/sdcard/Black/Black-V3-Cubo.iso diagnostico=ativo\n'
             f'MODELO recurso_completo caminho=D:/levels/level_00/fpguns/bg1_pst.db tamanho={len(self.dados)} crc32={crc}')
        self.assertIn('Fonte confirmada',verificar(log,self.dados,'Black-V3-Cubo.iso','levels/level_00/fpguns/bg1_pst.db'))
        for texto in (log.replace(crc,'00000000'),log.replace('Black-V3-Cubo','Outro'),log.splitlines()[0]):
            with self.assertRaises(ValueError):verificar(texto,self.dados,'Black-V3-Cubo.iso','levels/level_00/fpguns/bg1_pst.db')

    def test_rollback_relatorio_inacessivel(self):
        with self.assertRaises(OSError):v3.substituir(self.original,self.exemplo,self.p/'x.db',self.p/'ausente/x.json',True)
        self.assertFalse((self.p/'x.db').exists())


if __name__=='__main__':
    suite=unittest.defaultTestLoader.loadTestsFromTestCase(TestV3)
    resultado=unittest.TextTestRunner(verbosity=2).run(suite)
    raise SystemExit(0 if resultado.wasSuccessful() else 1)
