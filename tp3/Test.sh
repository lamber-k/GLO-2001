#!/bin/bash

DEBUG=""
BIN="./ufs_complet"

# Pour toujours faire les tests à partir d'un disque identique à l'original
echo "Je copie le fichier DisqueVirtuel.dat.orig vers DisqueVirtuel.dat" 
cp DisqueVirtuel.dat.orig DisqueVirtuel.dat

echo
echo "--------------------------------------------------------------------"
echo "                     montrer le contenu du disque"
echo "--------------------------------------------------------------------"
$DEBUG $BIN ls /
$DEBUG $BIN ls /doc
$DEBUG $BIN ls /doc/tmp
$DEBUG $BIN ls /doc/tmp/subtmp
$DEBUG $BIN ls /rep
$DEBUG $BIN ls /Bonjour

echo
echo
echo "--------------------------------------------------------------------"
echo "Tester les cas ou ls est fait sur un repertoire non-existant ou un fichier ordinaire"
echo "--------------------------------------------------------------------"
$DEBUG $BIN ls /mauvais
$DEBUG $BIN ls /b.txt

echo
echo
echo "--------------------------------------------------------------------"
echo "Maintenant on verifie que les bons b.txt sont accédés"
echo "Les numéros d'i-nodes doivent être différents"
echo "--------------------------------------------------------------------"
$DEBUG $BIN stat /doc/tmp/subtmp/b.txt 
$DEBUG $BIN stat /b.txt 

echo
echo
echo "--------------------------------------------------------------------"
echo "    test de lecture d'un repertoire, fichier inexistant ou vide"
echo "--------------------------------------------------------------------"
$DEBUG $BIN read /rep 0 10
$DEBUG $BIN read /toto.txt 0 10
$DEBUG $BIN read /b.txt 0 10

echo
echo
echo "--------------------------------------------------------------------"
echo "                  test d'ecriture de 40 caracteres"
echo "--------------------------------------------------------------------"
$DEBUG $BIN write /b.txt "1234567890ABCDEFGHIJ1234567890ABCDEFGHIJ" 0 
$DEBUG $BIN stat /b.txt
$DEBUG $BIN blockfree

echo
echo
echo "--------------------------------------------------------------------"
echo "                          tests de lecture"
echo "--------------------------------------------------------------------"

$DEBUG $BIN read /b.txt 0 30
$DEBUG $BIN read /b.txt 0 20
$DEBUG $BIN read /b.txt 0 10
$DEBUG $BIN read /b.txt 10 30
$DEBUG $BIN read /b.txt 10 5

echo
echo
echo "--------------------------------------------------------------------"
echo "      test d'ecriture de 1 caracteres en milieu de fichier"
echo "--------------------------------------------------------------------"
$DEBUG $BIN write /b.txt "-" 14 
$DEBUG $BIN stat /b.txt
$DEBUG $BIN blockfree  
$DEBUG $BIN read /b.txt 0 20

echo
echo
echo "--------------------------------------------------------------------"
echo "test d'ecriture de 1 caracteres, mais trop loin"
echo "--------------------------------------------------------------------"
$DEBUG $BIN write /b.txt "X" 41 
$DEBUG $BIN read /b.txt 0 50

echo
echo
echo "--------------------------------------------------------------------"
echo "   test d'ecriture exactement après le dernier caractère du fichier"
echo "--------------------------------------------------------------------"
$DEBUG $BIN write /b.txt "+" 40 
$DEBUG $BIN stat /b.txt
$DEBUG $BIN read /b.txt 0 50

echo
echo
echo "--------------------------------------------------------------------"
echo "test d'ecriture augmentant la taille du fichier, mais sans saisie de nouveau bloc"
echo "--------------------------------------------------------------------"
$DEBUG $BIN write /b.txt "abcdefghij" 40 
$DEBUG $BIN stat /b.txt
$DEBUG $BIN blockfree; N_FREEBLOCK=$?;  
$DEBUG $BIN read /b.txt 0 60

echo
echo
echo "--------------------------------------------------------------------"
echo "  test d'ecriture qui doit provoquer la saisie de 2 nouveaux blocs"
echo "--------------------------------------------------------------------"
$DEBUG $BIN write /b.txt "abcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJ" 0 
$DEBUG $BIN stat /b.txt
let "N_FREEBLOCK=$N_FREEBLOCK-2"
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre:"
$DEBUG $BIN blockfree  

echo
echo
echo "-----------------------------------------------------------------------"
echo "  test de lecture dans le fichier plus gros, qui chevauche deux blocs"
echo "-----------------------------------------------------------------------"
$DEBUG $BIN read /b.txt 0 600
$DEBUG $BIN read /b.txt 500 30
$DEBUG $BIN read /b.txt 500 100

echo
echo
echo "--------------------------------------------------------------------"
echo "                    Tester la commande hardlink"
echo "--------------------------------------------------------------------"
echo "Le nombre de blocs libre ne doit pas changer"
$DEBUG $BIN blockfree; N_FREEBLOCK=$?;
echo -e "\nDoit réussir:"
$DEBUG $BIN hardlink /b.txt /hlnb.txt
echo -e "\nDoit échouer avec -2, car hlnb.txt existe déjà:"
$DEBUG $BIN hardlink /b.txt /hlnb.txt
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre:"
$DEBUG $BIN blockfree
echo -e "\nDoit afficher les mêmes numéros d'i-node pour /b.txt et /hlnb.txt:"
$DEBUG $BIN ls /

echo
echo
echo "--------------------------------------------------------------------"
echo "                    Tester la commande unlink"
echo "--------------------------------------------------------------------"
$DEBUG $BIN unlink /b.txt
$DEBUG $BIN ls /
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre, car l'inode est toujours détenu par hlnb.txt:"
$DEBUG $BIN blockfree
$DEBUG $BIN unlink /hlnb.txt
$DEBUG $BIN ls /
let "N_FREEBLOCK=$N_FREEBLOCK+1"
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre, car l'inode a été libéré:"
$DEBUG $BIN blockfree
echo -e "\nDoit échouer avec -1, car /b.txt n'existe plus:"
$DEBUG $BIN unlink /b.txt
echo -e "\nDoit échouer avec -1, car /doc/tmp/b.txt n'existe pas:"
$DEBUG $BIN unlink /doc/tmp/b.txt 
$DEBUG $BIN unlink /doc/tmp/subtmp/b.txt 
$DEBUG $BIN ls /doc/tmp/subtmp
echo -e "\nDoit échouer avec -2, car /doc est un répertoire:"
$DEBUG $BIN unlink /doc

echo
echo
echo "--------------------------------------------------------------------"
echo "                    Tester la commande rmdir"
echo "--------------------------------------------------------------------"
$DEBUG $BIN blockfree; N_FREEBLOCK=$?;
$DEBUG $BIN rmdir /rep
$DEBUG $BIN ls /
let "N_FREEBLOCK=$N_FREEBLOCK+1"
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre, car le fichier répertoire a été libéré:"
$DEBUG $BIN blockfree
echo -e "\nDoit échouer avec -3, car /doc n'est pas vide:."
$DEBUG $BIN rmdir /doc
$DEBUG $BIN ls /
echo -e "\nDoit échouer avec -3, car /doc/tmp n'est pas vide:"
$DEBUG $BIN rmdir /doc/tmp

echo
echo
echo "--------------------------------------------------------------------"
echo "              Tester la création d'un fichier vide"
echo "--------------------------------------------------------------------"
$DEBUG $BIN create /Doge.wow
$DEBUG $BIN ls /
$DEBUG $BIN create /doc/tmp/new.txt 
$DEBUG $BIN ls /
$DEBUG $BIN ls /doc/tmp

echo
echo
echo "--------------------------------------------------------------------"
echo "          Tester la fonction rename sur fichier ordinaire"
echo "--------------------------------------------------------------------"
$DEBUG $BIN rename /Bonjour/LesAmis.txt /Bonjour/OncleG.txt
$DEBUG $BIN ls /Bonjour
$DEBUG $BIN rename /Bonjour/OncleG.txt /DansRoot.txt
$DEBUG $BIN ls /Bonjour
$DEBUG $BIN ls /

echo
echo
echo "--------------------------------------------------------------------"
echo "                Tester la création d'un répertoire"
echo "--------------------------------------------------------------------"
$DEBUG $BIN blockfree; N_FREEBLOCK=$?;
$DEBUG $BIN ls /Bonjour
$DEBUG $BIN mkdir /Bonjour/newdir
let "N_FREEBLOCK=$N_FREEBLOCK-1"
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre, car le fichier répertoire a utilisé un bloc:"
$DEBUG $BIN blockfree
echo -e "\nOn vérifie que le nombre de lien nlink pour /Bonjour augmente de 1, à cause du sous-répertoire newdir:"
$DEBUG $BIN ls /Bonjour
$DEBUG $BIN ls /

echo
echo
echo "--------------------------------------------------------------------"
echo "            Tester la fonction rename sur répertoire"
echo "--------------------------------------------------------------------"
$DEBUG $BIN ls /Bonjour
$DEBUG $BIN ls /doc
$DEBUG $BIN ls /
$DEBUG $BIN rename /doc/tmp /Bonjour/tmpmv
echo -e "\nOn vérifie que le nombre de lien pour /Bonjour augmente de 1 et qu'il diminue de 1 pour /doc:"
$DEBUG $BIN ls /Bonjour
$DEBUG $BIN ls /doc
$DEBUG $BIN ls /
echo -e "\nOn vérifie que le sous-réperoire tmpmv contient encore subtmp et new.txt:"
$DEBUG $BIN ls /Bonjour/tmpmv
echo -e "\nOn vérifie que le nombre de lien vers ce même répertoire n'augmente pas si on répète l'opération:"
$DEBUG $BIN rename /Bonjour/tmpmv /Bonjour/tmpmv2
$DEBUG $BIN ls /Bonjour
$DEBUG $BIN rename /Bonjour/tmpmv2 /Bonjour/tmpmv3
$DEBUG $BIN ls /Bonjour


