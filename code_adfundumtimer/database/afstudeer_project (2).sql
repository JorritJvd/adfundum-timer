-- phpMyAdmin SQL Dump
-- version 5.2.2
-- https://www.phpmyadmin.net/
--
-- Host: db
-- Gegenereerd op: 06 jun 2025 om 19:44
-- Serverversie: 10.6.21-MariaDB-ubu2004
-- PHP-versie: 8.2.28

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `afstudeer_project`
--

-- --------------------------------------------------------

--
-- Tabelstructuur voor tabel `deelnemers`
--

CREATE TABLE `deelnemers` (
  `id` int(11) NOT NULL,
  `naam` varchar(100) NOT NULL,
  `email` varchar(100) NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT current_timestamp()
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;



CREATE TABLE `drinksessies` (
  `id` int(11) NOT NULL,
  `deelnemer_id` int(11) NOT NULL,
  `drinktijd` decimal(6,2) NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT current_timestamp()
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;



--
-- Tabelstructuur voor tabel `glazen`
--

CREATE TABLE `glazen` (
  `id` int(11) NOT NULL,
  `uid` varchar(50) NOT NULL,
  `deelnemer_id` int(11) DEFAULT NULL,
  `created_at` timestamp NOT NULL DEFAULT current_timestamp()
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Gegevens worden geëxporteerd voor tabel `glazen`
--

INSERT INTO `glazen` (`id`, `uid`, `deelnemer_id`, `created_at`) VALUES
(1, '1DD2F3CA930000', NULL, '2025-05-24 14:20:13'),
(2, '1D4FE1CA930000', NULL, '2025-05-24 14:20:13'),
(3, '1D8621CB930000', NULL, '2025-05-24 14:20:13'),
(4, 'CD52633F', NULL, '2025-06-03 19:41:22'),
(5, '054D633F', NULL, '2025-06-03 19:41:22'),
(6, '4345633F', NULL, '2025-06-03 19:41:22'),
(7, '9B3F633F', NULL, '2025-06-03 19:41:22'),
(8, '3936633F', NULL, '2025-06-03 19:41:22'),
(9, 'E03B633F', NULL, '2025-06-03 19:41:22'),
(10, '4745633F', NULL, '2025-06-03 19:41:22');

--
-- Indexen voor geëxporteerde tabellen
--

--
-- Indexen voor tabel `deelnemers`
--
ALTER TABLE `deelnemers`
  ADD PRIMARY KEY (`id`);

--
-- Indexen voor tabel `drinksessies`
--
ALTER TABLE `drinksessies`
  ADD PRIMARY KEY (`id`),
  ADD KEY `fk_sessies_deelnemer` (`deelnemer_id`);

--
-- Indexen voor tabel `glazen`
--
ALTER TABLE `glazen`
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `uid` (`uid`),
  ADD KEY `fk_glazen_deelnemer` (`deelnemer_id`);

--
-- AUTO_INCREMENT voor geëxporteerde tabellen
--

--
-- AUTO_INCREMENT voor een tabel `deelnemers`
--
ALTER TABLE `deelnemers`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=35;

--
-- AUTO_INCREMENT voor een tabel `drinksessies`
--
ALTER TABLE `drinksessies`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=228;

--
-- AUTO_INCREMENT voor een tabel `glazen`
--
ALTER TABLE `glazen`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=11;

--
-- Beperkingen voor geëxporteerde tabellen
--

--
-- Beperkingen voor tabel `drinksessies`
--
ALTER TABLE `drinksessies`
  ADD CONSTRAINT `fk_sessies_deelnemer` FOREIGN KEY (`deelnemer_id`) REFERENCES `deelnemers` (`id`) ON DELETE CASCADE;

--
-- Beperkingen voor tabel `glazen`
--
ALTER TABLE `glazen`
  ADD CONSTRAINT `fk_glazen_deelnemer` FOREIGN KEY (`deelnemer_id`) REFERENCES `deelnemers` (`id`) ON DELETE SET NULL;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
