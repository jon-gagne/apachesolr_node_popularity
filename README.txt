APACHE SOLR NODE POPULARITY MODULE
====================================

SUMMARY -----------------------------------------------------

Apache Solr Node Popularity extends Apache Solr Search Integration to include the popularity of a node into Solr’s search ranking.  This can greatly improve the relevancy of the search results by pulling frequently visited pages towards the top and pushing infrequently visited pages towards the bottom, which allows users to find what they are likely looking for easier and quicker.

For a link to the technical details, see the Documentation Section of the project page.


REQUIREMENTS ------------------------------------------------

- Apache Solr server
- Apache Solr Search Integration (apachesolr)
- Apache Solr Config Generation (apachesolr_confgen)


INSTALLATION & CONFIGURATION --------------------------------

- Install an Apache Solr server (3.6.x)
- Install and enable the Apache Solr Search Integration module (dev, or greater than 7.x-1.1)
- Install and enable the Apache Solr Config Generator module (dev, or greater than alpha2)
- Install and enable the Apache Solr Node Popularity module
- Goto "admin/config/search/apachesolr/confgen"
- Download all files for solr 3.5.x and 3.6.x as zip archive
- Extract files into Solr config directory (e.g., "../tomcat/solr/conf/")
- Configure the Apache Solr Search Integration module as documented 
  (includes adding a Solr search environment at "admin/config/search/apachesolr/settings")
- Goto "admin/config/search/apachesolr/apachesolr_popularity"
- Enable Apache Solr Node Popularity and enter the Solr data directory (e.g., "../tomcat/solr/data/")

Note: the Popular Pages report can be viewed at "admin/reports/popularpages"


CUSTOMIZATION -----------------------------------------------

- Configure Basic Settings  
- Optional: Enable and configure advanced settings 
  ("config/search/apachesolr/apachesolr_popularity/advanced_settings")


CONTACT -----------------------------------------------------

Developer: 
  Jonathan Gagne (jongagne) - http://drupal.org/user/2409764

This project was funded by:
  OPIN Software - http://www.opin.ca/

