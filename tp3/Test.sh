#!/bin/bash

DEBUG=""

# Pour toujours faire les tests à partir d'un disque identique à l'original
echo "Je copie le fichier DisqueVirtuel.dat.orig vers DisqueVirtuel.dat" 
cp DisqueVirtuel.dat.orig DisqueVirtuel.dat

echo
echo "--------------------------------------------------------------------"
echo "                     montrer le contenu du disque"
echo "--------------------------------------------------------------------"
$DEBUG ./ufs ls /
$DEBUG ./ufs ls /doc
$DEBUG ./ufs ls /doc/tmp
$DEBUG ./ufs ls /doc/tmp/subtmp
$DEBUG ./ufs ls /rep
$DEBUG ./ufs ls /Bonjour

echo
echo
echo "--------------------------------------------------------------------"
echo "Tester les cas ou ls est fait sur un repertoire non-existant ou un fichier ordinaire"
echo "--------------------------------------------------------------------"
$DEBUG ./ufs ls /mauvais
$DEBUG ./ufs ls /b.txt

echo
echo
echo "--------------------------------------------------------------------"
echo "Maintenant on verifie que les bons b.txt sont accédés"
echo "Les numéros d'i-nodes doivent être différents"
echo "--------------------------------------------------------------------"
$DEBUG ./ufs stat /doc/tmp/subtmp/b.txt 
$DEBUG ./ufs stat /b.txt 

echo
echo
echo "--------------------------------------------------------------------"
echo "    test de lecture d'un repertoire, fichier inexistant ou vide"
echo "--------------------------------------------------------------------"
$DEBUG ./ufs read /rep 0 10
$DEBUG ./ufs read /toto.txt 0 10
$DEBUG ./ufs read /b.txt 0 10

echo
echo
echo "--------------------------------------------------------------------"
echo "                  test d'ecriture de 40 caracteres"
echo "--------------------------------------------------------------------"
$DEBUG ./ufs write /b.txt "1234567890ABCDEFGHIJ1234567890ABCDEFGHIJ" 0 
$DEBUG ./ufs stat /b.txt
$DEBUG ./ufs blockfree

echo
echo
echo "--------------------------------------------------------------------"
echo "                          tests de lecture"
echo "--------------------------------------------------------------------"

$DEBUG ./ufs read /b.txt 0 30
$DEBUG ./ufs read /b.txt 0 20
$DEBUG ./ufs read /b.txt 0 10
$DEBUG ./ufs read /b.txt 10 30
$DEBUG ./ufs read /b.txt 10 5

echo
echo
echo "--------------------------------------------------------------------"
echo "      test d'ecriture de 1 caracteres en milieu de fichier"
echo "--------------------------------------------------------------------"
$DEBUG ./ufs write /b.txt "-" 14 
$DEBUG ./ufs stat /b.txt
$DEBUG ./ufs blockfree  
$DEBUG ./ufs read /b.txt 0 20

echo
echo
echo "--------------------------------------------------------------------"
echo "test d'ecriture de 1 caracteres, mais trop loin"
echo "--------------------------------------------------------------------"
$DEBUG ./ufs write /b.txt "X" 41 
$DEBUG ./ufs read /b.txt 0 50

echo
echo
echo "--------------------------------------------------------------------"
echo "   test d'ecriture exactement après le dernier caractère du fichier"
echo "--------------------------------------------------------------------"
$DEBUG ./ufs write /b.txt "+" 40 
$DEBUG ./ufs stat /b.txt
$DEBUG ./ufs read /b.txt 0 50

echo
echo
echo "--------------------------------------------------------------------"
echo "test d'ecriture augmentant la taille du fichier, mais sans saisie de nouveau bloc"
echo "--------------------------------------------------------------------"
$DEBUG ./ufs write /b.txt "abcdefghij" 40 
$DEBUG ./ufs stat /b.txt
$DEBUG ./ufs blockfree; N_FREEBLOCK=$?;  
$DEBUG ./ufs read /b.txt 0 60

echo
echo
echo "--------------------------------------------------------------------"
echo "  test d'ecriture qui doit provoquer la saisie de 2 nouveaux blocs"
echo "--------------------------------------------------------------------"
$DEBUG ./ufs write /b.txt "abcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJ" 0 
$DEBUG ./ufs stat /b.txt
let "N_FREEBLOCK=$N_FREEBLOCK-2"
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre:"
$DEBUG ./ufs blockfree  

echo
echo
echo "-----------------------------------------------------------------------"
echo "  test de lecture dans le fichier plus gros, qui chevauche deux blocs"
echo "-----------------------------------------------------------------------"
$DEBUG ./ufs read /b.txt 0 600
$DEBUG ./ufs read /b.txt 500 30
$DEBUG ./ufs read /b.txt 500 100

echo
echo
echo "--------------------------------------------------------------------"
echo "                    Tester la commande hardlink"
echo "--------------------------------------------------------------------"
echo "Le nombre de blocs libre ne doit pas changer"
$DEBUG ./ufs blockfree; N_FREEBLOCK=$?;
echo -e "\nDoit réussir:"
$DEBUG ./ufs hardlink /b.txt /hlnb.txt
echo -e "\nDoit échouer avec -2, car hlnb.txt existe déjà:"
$DEBUG ./ufs hardlink /b.txt /hlnb.txt
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre:"
$DEBUG ./ufs blockfree
echo -e "\nDoit afficher les mêmes numéros d'i-node pour /b.txt et /hlnb.txt:"
$DEBUG ./ufs ls /

echo
echo
echo "--------------------------------------------------------------------"
echo "                    Tester la commande unlink"
echo "--------------------------------------------------------------------"
$DEBUG ./ufs unlink /b.txt
$DEBUG ./ufs ls /
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre, car l'inode est toujours détenu par hlnb.txt:"
$DEBUG ./ufs blockfree
$DEBUG ./ufs unlink /hlnb.txt
$DEBUG ./ufs ls /
let "N_FREEBLOCK=$N_FREEBLOCK+1"
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre, car l'inode a été libéré:"
$DEBUG ./ufs blockfree
echo -e "\nDoit échouer avec -1, car /b.txt n'existe plus:"
$DEBUG ./ufs unlink /b.txt
echo -e "\nDoit échouer avec -1, car /doc/tmp/b.txt n'existe pas:"
$DEBUG ./ufs unlink /doc/tmp/b.txt 
$DEBUG ./ufs unlink /doc/tmp/subtmp/b.txt 
$DEBUG ./ufs ls /doc/tmp/subtmp
echo -e "\nDoit échouer avec -2, car /doc est un répertoire:"
$DEBUG ./ufs unlink /doc

echo
echo
echo "--------------------------------------------------------------------"
echo "                    Tester la commande rmdir"
echo "--------------------------------------------------------------------"
$DEBUG ./ufs blockfree; N_FREEBLOCK=$?;
$DEBUG ./ufs rmdir /rep
$DEBUG ./ufs ls /
let "N_FREEBLOCK=$N_FREEBLOCK+1"
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre, car le fichier répertoire a été libéré:"
$DEBUG ./ufs blockfree
echo -e "\nDoit échouer avec -3, car /doc n'est pas vide:."
$DEBUG ./ufs rmdir /doc
$DEBUG ./ufs ls /
echo -e "\nDoit échouer avec -3, car /doc/tmp n'est pas vide:"
$DEBUG ./ufs rmdir /doc/tmp

echo
echo
echo "--------------------------------------------------------------------"
echo "              Tester la création d'un fichier vide"
echo "--------------------------------------------------------------------"
$DEBUG ./ufs create /Doge.wow
$DEBUG ./ufs ls /
$DEBUG ./ufs create /doc/tmp/new.txt 
$DEBUG ./ufs ls /
$DEBUG ./ufs ls /doc/tmp

echo
echo
echo "--------------------------------------------------------------------"
echo "          Tester la fonction rename sur fichier ordinaire"
echo "--------------------------------------------------------------------"
$DEBUG ./ufs rename /Bonjour/LesAmis.txt /Bonjour/OncleG.txt
$DEBUG ./ufs ls /Bonjour
$DEBUG ./ufs rename /Bonjour/OncleG.txt /DansRoot.txt
$DEBUG ./ufs ls /Bonjour
$DEBUG ./ufs ls /

echo
echo
echo "--------------------------------------------------------------------"
echo "                Tester la création d'un répertoire"
echo "--------------------------------------------------------------------"
$DEBUG ./ufs blockfree; N_FREEBLOCK=$?;
$DEBUG ./ufs ls /Bonjour
$DEBUG ./ufs mkdir /Bonjour/newdir
let "N_FREEBLOCK=$N_FREEBLOCK-1"
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre, car le fichier répertoire a utilisé un bloc:"
$DEBUG ./ufs blockfree
echo -e "\nOn vérifie que le nombre de lien nlink pour /Bonjour augmente de 1, à cause du sous-répertoire newdir:"
$DEBUG ./ufs ls /Bonjour
$DEBUG ./ufs ls /

echo
echo
echo "--------------------------------------------------------------------"
echo "            Tester la fonction rename sur répertoire"
echo "--------------------------------------------------------------------"
$DEBUG ./ufs ls /Bonjour
$DEBUG ./ufs ls /doc
$DEBUG ./ufs ls /
$DEBUG ./ufs rename /doc/tmp /Bonjour/tmpmv
echo -e "\nOn vérifie que le nombre de lien pour /Bonjour augmente de 1 et qu'il diminue de 1 pour /doc:"
$DEBUG ./ufs ls /Bonjour
$DEBUG ./ufs ls /doc
$DEBUG ./ufs ls /
echo -e "\nOn vérifie que le sous-réperoire tmpmv contient encore subtmp et new.txt:"
$DEBUG ./ufs ls /Bonjour/tmpmv
echo -e "\nOn vérifie que le nombre de lien vers ce même répertoire n'augmente pas si on répète l'opération:"
$DEBUG ./ufs rename /Bonjour/tmpmv /Bonjour/tmpmv2
$DEBUG ./ufs ls /Bonjour
$DEBUG ./ufs rename /Bonjour/tmpmv2 /Bonjour/tmpmv3
$DEBUG ./ufs ls /Bonjour


