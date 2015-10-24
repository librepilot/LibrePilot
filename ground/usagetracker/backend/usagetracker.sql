-- phpMyAdmin SQL Dump
-- version 4.4.14.1
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: Sep 13, 2015 at 03:06 PM
-- Server version: 10.0.20-MariaDB
-- PHP Version: 5.6.13

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `usagetracker`
--

-- --------------------------------------------------------

--
-- Table structure for table `usagetracker`
--

CREATE TABLE IF NOT EXISTS `usagetracker` (
  `id` int(11) unsigned NOT NULL,
  `first_date` int(11) unsigned NOT NULL,
  `last_date` int(11) unsigned NOT NULL,
  `ip` varchar(8) COLLATE utf8_bin NOT NULL,
  `count` smallint(4) unsigned NOT NULL DEFAULT '1',
  `data` varchar(2000) COLLATE utf8_bin NOT NULL,
  `hash` varchar(32) COLLATE utf8_bin NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;

--
-- Indexes for dumped tables
--

--
-- Indexes for table `usagetracker`
--
ALTER TABLE `usagetracker`
  ADD PRIMARY KEY (`id`),
  ADD KEY `last_date` (`last_date`),
  ADD KEY `hash` (`hash`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `usagetracker`
--
ALTER TABLE `usagetracker`
  MODIFY `id` int(11) unsigned NOT NULL AUTO_INCREMENT;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
