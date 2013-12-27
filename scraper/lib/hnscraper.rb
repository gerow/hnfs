require 'nokogiri'
require 'open-uri'
require 'uri'

class HNScraper
  # This is all super hacky, but really what scraper isn't?
  def initialize params={}
    @url = params[:url] || "https://news.ycombinator.com/"
  end

  def front_page
    doc = Nokogiri::HTML(open(@url))
    p doc
    sliced_story_xmls = doc.css("table>tr>td>table>tr").each_slice(3).to_a[0..29]
    stories = sliced_story_xmls.map{|story_xmls|
      story = {}
      story[:url] = story_xmls[1].css("td.title>a").attr("href").value
      story_uri = URI(story[:url])
      if not story_uri.host
        story[:url] = @url + story[:url]
      end
      story[:title] = story_xmls[1].css("td.title>a").text
      story[:user] = story_xmls[2].css("td.subtext>a")[0].text
      story[:comments] = story_xmls[2].css("td.subtext>a")[1].attr("href")
      story[:comments] = @url + story[:comments]
      story
    }
    return stories
  end
end
