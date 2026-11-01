![CI](https://github.com/ovidiuandreipatrascu/proiect_map/actions/workflows/ci.yml/badge.svg)
# Proiect MAP

Proiect individual la disciplina Metode avansate de programare, anul universitar 2026-2027.

## Autor

- **Nume:** Patrascu Ovidiu-Andrei
- **Grupa:** 2.1
- **Marca:** LH716192
- **Tema:** 1 - Agenda de contacte

## Descriere

Aplicatia gestioneaza o agenda digitala de contacte, permitand adaugarea, cautarea, editarea si stergerea rapida a contactelor. Proiectul ofera un API REST rapid si eficient scris in C++.

## Tehnologii

C++20 cu cpp-httplib si nlohmann/json

## Rulare

```
docker build -t map-proiect .
docker run -d -p 8080:8080 map-proiect
```

Aplicatia asculta pe portul 8080. Verificati:

```
curl http://localhost:8080/health
curl http://localhost:8080/version
```

## Testare

```
Aplicatia ruleaza ca microserviciu containerizat si expune endpoint-uri REST pentru interactiunea cu clientii.
cmake -B build -DBUILD_TESTS=ON
cmake --build build -j
./build/tests
```

## Rutele implementate

| Ruta | Metoda | Descriere |
|---|---|---|
| `/health` | GET | Starea serviciului |
| `/version` | GET | Versiunea si commit-ul din care a fost construita imaginea |
| `/` | GET | Pagina de prezentare |
| `/reset` | POST | Goleste datele din memorie |
| `/contacts` | POST | Adauga un contact in agenda (valideaza datele si unicitatea emailului) |

## Decizii de implementare

- Validarile campurilor (lungime nume, format email si telefon, categorii permise) sunt izolate in `app.hpp` pentru a permite testarea unitara fara pornirea serverului HTTP.
- Verificarea unicitatii adresei de email se face case-insensitive (fara diferenta intre majuscule si minuscule), pastrand totusi forma originala a textului la salvare si citire.