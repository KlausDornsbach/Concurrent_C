#!/bin/bash
# Usage: grade dir_or_archive [output]

# Ensure realpath 
realpath . &>/dev/null
HAD_REALPATH=$(test "$?" -eq 127 && echo no || echo yes)
if [ "$HAD_REALPATH" = "no" ]; then
  cat > /tmp/realpath-grade.c <<EOF
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
  char* path = argv[1];
  char result[8192];
  memset(result, 0, 8192);

  if (argc == 1) {
      printf("Usage: %s path\n", argv[0]);
      return 2;
  }
  
  if (realpath(path, result)) {
    printf("%s\n", result);
    return 0;
  } else {
    printf("%s\n", argv[1]);
    return 1;
  }
}
EOF
  cc -o /tmp/realpath-grade /tmp/realpath-grade.c
  function realpath () {
    /tmp/realpath-grade $@
  }
fi

INFILE=$1
if [ -z "$INFILE" ]; then
  CWD_KBS=$(du -d 0 . | cut -f 1)
  if [ -n "$CWD_KBS" -a "$CWD_KBS" -gt 20000 ]; then
    echo "Chamado sem argumentos."\
         "Supus que \".\" deve ser avaliado, mas esse diretório é muito grande!"\
         "Se realmente deseja avaliar \".\", execute $0 ."
    exit 1
  fi
fi
test -z "$INFILE" && INFILE="."
INFILE=$(realpath "$INFILE")
# grades.csv is optional
OUTPUT=""
test -z "$2" || OUTPUT=$(realpath "$2")
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
# Absolute path to this script
THEPACK="${DIR}/$(basename "${BASH_SOURCE[0]}")"
STARTDIR=$(pwd)

# Split basename and extension
BASE=$(basename "$INFILE")
EXT=""
if [ ! -d "$INFILE" ]; then
  BASE=$(echo $(basename "$INFILE") | sed -E 's/^(.*)(\.(c|zip|(tar\.)?(gz|bz2|xz)))$/\1/g')
  EXT=$(echo  $(basename "$INFILE") | sed -E 's/^(.*)(\.(c|zip|(tar\.)?(gz|bz2|xz)))$/\2/g')
fi

# Setup working dir
rm -fr "/tmp/$BASE-test" || true
mkdir "/tmp/$BASE-test" || ( echo "Could not mkdir /tmp/$BASE-test"; exit 1 )
UNPACK_ROOT="/tmp/$BASE-test"
cd "$UNPACK_ROOT"

function cleanup () {
  test -n "$1" && echo "$1"
  cd "$STARTDIR"
  rm -fr "/tmp/$BASE-test"
  test "$HAD_REALPATH" = "yes" || rm /tmp/realpath-grade* &>/dev/null
  return 1 # helps with precedence
}

# Avoid messing up with the running user's home directory
# Not entirely safe, running as another user is recommended
export HOME=.

# Check if file is a tar archive
ISTAR=no
if [ ! -d "$INFILE" ]; then
  ISTAR=$( (tar tf "$INFILE" &> /dev/null && echo yes) || echo no )
fi

# Unpack the submission (or copy the dir)
if [ -d "$INFILE" ]; then
  cp -r "$INFILE" . || cleanup || exit 1 
elif [ "$EXT" = ".c" ]; then
  echo "Corrigindo um único arquivo .c. O recomendado é corrigir uma pasta ou  arquivo .tar.{gz,bz2,xz}, zip, como enviado ao moodle"
  mkdir c-files || cleanup || exit 1
  cp "$INFILE" c-files/ ||  cleanup || exit 1
elif [ "$EXT" = ".zip" ]; then
  unzip "$INFILE" || cleanup || exit 1
elif [ "$EXT" = ".tar.gz" ]; then
  tar zxf "$INFILE" || cleanup || exit 1
elif [ "$EXT" = ".tar.bz2" ]; then
  tar jxf "$INFILE" || cleanup || exit 1
elif [ "$EXT" = ".tar.xz" ]; then
  tar Jxf "$INFILE" || cleanup || exit 1
elif [ "$EXT" = ".gz" -a "$ISTAR" = "yes" ]; then
  tar zxf "$INFILE" || cleanup || exit 1
elif [ "$EXT" = ".gz" -a "$ISTAR" = "no" ]; then
  gzip -cdk "$INFILE" > "$BASE" || cleanup || exit 1
elif [ "$EXT" = ".bz2" -a "$ISTAR" = "yes"  ]; then
  tar jxf "$INFILE" || cleanup || exit 1
elif [ "$EXT" = ".bz2" -a "$ISTAR" = "no" ]; then
  bzip2 -cdk "$INFILE" > "$BASE" || cleanup || exit 1
elif [ "$EXT" = ".xz" -a "$ISTAR" = "yes"  ]; then
  tar Jxf "$INFILE" || cleanup || exit 1
elif [ "$EXT" = ".xz" -a "$ISTAR" = "no" ]; then
  xz -cdk "$INFILE" > "$BASE" || cleanup || exit 1
else
  echo "Unknown extension $EXT"; cleanup; exit 1
fi

# There must be exactly one top-level dir inside the submission
# As a fallback, if there is no directory, will work directly on 
# tmp/$BASE-test, but in this case there must be files! 
function get-legit-dirs  {
  find . -mindepth 1 -maxdepth 1 -type d | grep -vE '^\./__MACOS' | grep -vE '^\./\.'
}
NDIRS=$(get-legit-dirs | wc -l)
test "$NDIRS" -lt 2 || \
  cleanup "Malformed archive! Expected exactly one directory, found $NDIRS" || exit 1
test  "$NDIRS" -eq  1 -o  "$(find . -mindepth 1 -maxdepth 1 -type f | wc -l)" -gt 0  || \
  cleanup "Empty archive!" || exit 1
if [ "$NDIRS" -eq 1 ]; then #only cd if there is a dir
  cd "$(get-legit-dirs)"
fi

# Unpack the testbench
tail -n +$(($(grep -ahn  '^__TESTBENCH_MARKER__' "$THEPACK" | cut -f1 -d:) +1)) "$THEPACK" | tar zx
cd testbench || cleanup || exit 1

# Deploy additional binaries so that validate.sh can use them
test "$HAD_REALPATH" = "yes" || cp /tmp/realpath-grade "tools/realpath"
cc -std=c11 tools/wrap-function.c -o tools/wrap-function \
  || echo "Compilation of wrap-function.c failed. If you are on a Mac, brace for impact"
export PATH="$PATH:$(realpath "tools")"

# Run validate
(./validate.sh 2>&1 | tee validate.log) || cleanup || exit 1

# Write output file
if [ -n "$OUTPUT" ]; then
  #write grade
  echo "@@@###grade:" > result
  cat grade >> result || cleanup || exit 1
  #write feedback, falling back to validate.log
  echo "@@@###feedback:" >> result
  (test -f feedback && cat feedback >> result) || \
    (test -f validate.log && cat validate.log >> result) || \
    cleanup "No feedback file!" || exit 1
  #Copy result to output
  test ! -d "$OUTPUT" || cleanup "$OUTPUT is a directory!" || exit 1
  rm -f "$OUTPUT"
  cp result "$OUTPUT"
fi

if ( ! grep -E -- '-[0-9]+' grade &> /dev/null ); then
   echo -e "Grade for $BASE$EXT: $(cat grade)"
fi

cleanup || true

exit 0

__TESTBENCH_MARKER__
�      ��R�Fv������/\6��!�gv*����ZL\Bjc�ȒG sٗ���}H�V�)����9�-��0��&���>}�V;fQ|�|{��苍:���5�nl����t<j��Z�F����|To4[�G���X�G�V���ؕ͆�m��tę�� �/��nz���!Ƹ�/Ckd�ߎ���'qo���zc��뫭�GP��7�?��/����ʙJ���:�={��i��g/�^��~B���>��l�� �>�@�����BN� 0����}(�X0 ��=#�E0?��$�����Wn��w�q���[C��n[1hD�!��Qpm(g{�2	'9����"Ɵ�
�%N�/����qc���{�A1���:#�������h���!�c׷��a��(vܠ6�.�S�럏�9�{V��9������^��`8��B/���U����`���@�VI�d�ӣ�U���#d#ϲl!�
�
�p�!�c��2tc6�Fqk̮h�4�\_�Vxng���i��i��J'i�����k�#�m�Wdy����Z�#:��"�tW��$��!f>�d��!��.��DAڬ}�z�
T9j����\B��Iu?'E��6��R� �W!E*��l�X_̖�^��}ǂ~j,Ø ���A9��
d�	�6�jCC�j��m�z��z��T�&w��-�1� mIPx���K6pA�p�my^`�*���|�3d�r����852���*��oE�"#71��p�l#�=�I�$��8��Y�,�����$�v��#���?��ܓ�L�0��y�.'�a�)��՜�C�"W���QЊ�H?�tc8��'��n��{���W9���is�nE�Z6&T���'?�?��nMJ��ϊ�Y
V�<��y��z��d��E5{f�F��A��1�OFӍŝf�Z�_a����S����b�eu���_Q�,8�nW?�%\<��jK�8����8��k+�g�y�0������"N[3����9+����]��m���-�'#��6I��-
�ռ��W�ϒ{�WUt3�،P���B)�.�*�r�(���gU�'2�eS�C�
�.U��+�V2}(�
�Y��R�N��{T$�4#�yOG��@6����i}r�l�Fp���9�����
G���ǝ����z8�|��⠃.�����K`@TAH���֕�T��������"u��F���@&7�~OM����t�ͼ�:�k�}��m�1�k�k��s���^	T�OY��x>��j�#�����N_u:��P�r���V��y< 1�<@��{_u�J�g��+�fz$�J��E�,ˑ�UȠ�(�A��������U4�Q�[{�Ⱦ�˅���~���e_j���<���3�ɽH{$���
��3�1����I	���DD�\ ]Ԩ��q�Z��UH����D��c�#���-�A �[piE\�1J��qR�1���`%�(ӑ�\�{�a��"�����(9���Њ��͠te6���L'WP�)����ݝ�%��:	RIEȁ��%"��[��������
�F��$|��!s��KXЧw��܎�U�Q�u�������:��q�jq}e\m��z�9����Q	��/u�K�˩�]�
qV��T_ښ���]nK-�ж�Z8� w�OFA�G��s
�b�{lCi�V�Q�,�T�k���δ��R��7dNn_b
�[��u)#kl�~M�0�t�]^6�t��ھ��	�>��4��ħL����ΐ>��vO�m]rZ�-s@q �M�]�XyS'(��a�?ݔY�)>�$�3��_(���/h���Z3/-7�?���ml��������2����{t����?�m����/_��-����y��Zz���}����vy�D��Z9�����@Vv��'��/]qO��P�8���cleN��0�����=�V������<��.��aE׿\0bw�x�bш�����?�d� ��Z�F��Sy�����c,��tv�x'�S֏��Ul����j��ŝ����6�*�h�{Ӿ[�x(��,�M�$iY�'���[�ٚ������5�KϏv�:��������2��,�����0�Ӕ}�߹QDU���#�:������CE?M�1�̲�܄I���\�ʕ��5F)��8@D��/��� ���N�d��)�8�p������s��u���q�����Ao�]����`�ў<y����rcSC��62c������f9�mt��S���3ozF���'[�G��e]Oa�$f�D&Y8�q�y�� ����q�-��f
���0nģ$JM�8<*�C�x�J��m�������]ϋ�V~���Bpt�/�779[��$TKe� ���gu�_�(�'(n"�G+����F]΢�  ��'��I�(Ҫ n=��#w����E!�*�S&S���#Ы�K$���[�j_՚��VN�f�(��;�>+�GH������x,�7����!"�����)��aCY���\+Bdf���n�^_Y9_24�.$����������幎�Z4�g����Fku����5����!F����j"����F$f!���/|��(Z>t�#�^ڝ����6����u~�%��9Kx�Z�d��7���{�H"hf9�J��'�'��4(?ɛ��m�}[x��0:�9��0���¨?�>k6��!�R:e�[zO�!��٢4�XMԹ)9���V��)��JEy�]���5�m�v�^��4� �1ch����Y;�R@O%��a��J\<�(�������]K�T��	l�X�� ͬ�I��q��X���:8�����#�)�Č�����+�����j��*+�(΢RX��J�cN�ք���Mѿz�+JԚ*QĆ�p�t���_��Qks�jmuh%�jQ���f\@J>�5�ɝ{`E&jO8�h������
aM9Z��?�#�pi��ɲJ55�PI%��8);K�Mp�:u�;��#�5p�����-'�$��L?}fy1:��(p��Z�4 ��I��b��8�(�S��t�M�>��_�.6����`�"= u��{�\�ڄ}6<�g�p�f��G����� rf�R��d��=E���j`ʥ���	W7v`�m�"���8N��)̇��9<r��8B<��`�>�vqw���a��n����=7F<�Y�jR���W�f��3���*z펊�Uj��u�V�	���d`'�O����ˏ�dxW�S!��u�s�	��2}�#�G�3��H�Jd�Oͽ1�� ���ի�"L���$�$)HXG�hq�D��_���_|wn�)��6f+@�o����@��$�]�	
TR#42pӔaA<`�G��\�~��on����iK��|�V�6g�/�?C����ƕ&�W��b��n�7#��̜��/c��zp�ϗ���'53�_��RL5S�w����j�yG���hLi�sU��g�������������������������������?��/��*� P  