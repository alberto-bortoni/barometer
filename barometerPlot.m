%% open-file
[FileName,PathName] = uigetfile('*.csv','Select the log file  ');
flight = input('Flight Number:  ','s');
date   = input('date (YY-MMM-DD):  ','s');
dest   = input('destination (ORG-SCL-DES):  ','s');
header = strcat(flight, string(' '), date, string(' '), dest);
cd(PathName);
data = csvread(FileName);
cd ..;

%% modify-array
%1- temp; 2-pressure; 3-humidity
data(:,1) = data(:,1)/5;
data(:,2) = data(:,2)*0.00986923;
datasize  = size(data);
datasize  = datasize(1,1);
t = linspace(0,datasize(1)/600,datasize(1));
xlim = t(datasize) + .25 - mod(t(datasize),.25);


%% plot
%first plot - temperature
figure
subplot(2,2,1);
plot(t,data(:,1),'Color',[0 .2 .6]);
ylabel('temperature [C]');
xlabel('time [h]');
ax = gca;
ax.XColor = [0 .2 .6];
ax.YColor = [0 .2 .6];
ax.XLim = [0 xlim];
ax.YLim = [18 32];
ax.XGrid = 'on';
ax.YGrid = 'on';

%second plot - humidity
subplot(2,2,2);
plot(t,data(:,3),'Color',[0 .6 0]);
xlabel('time [h]');
ylabel('humidity [%rel]');
ax = gca;
ax.XColor = [0 .6 0];
ax.YColor = [0 .6 0];
ax.XLim = [0 xlim];
ax.YLim = [5 70];
ax.XGrid = 'on';
ax.YGrid = 'on';

%third plot - pressure
subplot(2,2, [3 4]);
plot(t,data(:,2),'Color',[.4 .2 0]);
xlabel('time [h]');
ylabel('pressure [atm]');
ax = gca;
ax.XColor = [.4 .2 0];
ax.YColor = [.4 .2 0];
ax.XLim = [0 xlim];
ax.YLim = [.75 1.05];
ax.XGrid = 'on';
ax.YGrid = 'on';

shg;
p=mtit(char(header),'fontsize',18,'color',[0 0 0],'xoff',-.05,'yoff',.025);
saveas(gcf,char(header +'.png'));
