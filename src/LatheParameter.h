class LatheParameter {

    public:
        LatheParameter();

        // Persisted
        int backlash();
        void setBacklash(int value);

        int feedIndex();
        void setFeedIndex(int value);

        bool isMetricFeed();
        void setMetricFeed(bool value);

        bool isInvertFeed();
        void setInvertFeed(bool value);

        // Not persisted
        int rpm();
        int maxRpm();
        void setRpm(int value);
        void setMaxRpm(int value);

    private:
        int backlashValue;
        int feedIndexValue;
        bool metricFeedValue;
        bool invertFeedValue;

        int rpmValue;
        int maxRpmValue;
};