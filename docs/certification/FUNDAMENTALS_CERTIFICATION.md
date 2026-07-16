# ThemisDB Fundamentals Certification (TDF)

## Certification Overview

The **ThemisDB Fundamentals Certification (TDF)** is the entry-level certification that validates your foundational knowledge of ThemisDB database technology. This certification demonstrates that you understand core database concepts, can write basic AQL queries, perform installation and configuration, and implement basic security measures.

### Certification Details

- **Certification Code**: TDF
- **Level**: Entry/Foundation
- **Duration**: 90 minutes
- **Question Count**: 25-30 questions
- **Question Types**: Multiple choice, multiple select, and scenario-based
- **Passing Score**: 70% (18/25 minimum)
- **Validity**: 2 years
- **Prerequisites**: None
- **Exam Fee**: $150 USD
- **Retake Fee**: $75 USD
- **Language**: English (Spanish, Chinese, Japanese coming 2025)

---

## Target Audience

This certification is ideal for:

- **Software Developers** new to ThemisDB
- **Database Administrators** transitioning to ThemisDB
- **Data Engineers** working with multi-model data
- **System Architects** evaluating ThemisDB
- **DevOps Engineers** managing ThemisDB deployments
- **IT Professionals** expanding their database skills
- **Students** pursuing database technology careers
- **Technical Managers** overseeing ThemisDB projects

---

## Prerequisites

### Technical Prerequisites
- Basic understanding of database concepts (tables, queries, transactions)
- Familiarity with at least one programming language
- Basic command-line interface knowledge
- Understanding of client-server architecture

### Recommended Experience
- 3-6 months working with any database system
- Basic SQL knowledge (helpful but not required)
- Understanding of data modeling concepts

### No Prior Certification Required
This is the foundational certification with no prerequisites.

---

## Learning Objectives

Upon completing this certification, you will be able to:

### 1. Architecture and Concepts (20%)
- Explain ThemisDB's multi-model architecture
- Understand storage engine internals
- Describe ACID transaction properties
- Explain consistency models and isolation levels
- Understand sharding and partitioning strategies
- Describe replication mechanisms

### 2. Data Models (15%)
- Work with document data model
- Understand graph data structures
- Utilize key-value storage
- Implement time-series data patterns
- Use vector embeddings for AI/ML
- Apply appropriate model for use cases

### 3. AQL Query Language (25%)
- Write basic CRUD operations
- Construct SELECT queries with filters
- Use JOIN operations across models
- Implement aggregation functions
- Apply sorting and pagination
- Utilize subqueries and expressions

### 4. Installation and Configuration (15%)
- Install ThemisDB on various platforms
- Configure basic database parameters
- Set up network and port settings
- Manage configuration files
- Understand directory structure
- Perform basic troubleshooting

### 5. Security Fundamentals (15%)
- Configure authentication methods
- Implement basic authorization
- Create and manage users
- Assign permissions and roles
- Secure network connections
- Follow security best practices

### 6. Operations Basics (10%)
- Perform backup and restore operations
- Monitor basic database metrics
- Understand logging mechanisms
- Execute routine maintenance tasks
- Use administrative commands
- Troubleshoot common issues

---

## Core Concepts to Master

### 1. ThemisDB Architecture

#### System Architecture
- **Query Engine**: AQL parser, optimizer, and executor
- **Storage Engine**: Multi-model storage layer
- **Transaction Manager**: ACID compliance and isolation
- **Replication Layer**: Data consistency across nodes
- **Cache System**: Memory management and optimization

#### Multi-Model Capabilities
```
Document Model → JSON/BSON storage
Graph Model → Vertices and edges
Key-Value → Simple key-based access
Time-Series → Temporal data optimization
Vector → Embeddings for similarity search
```

#### Data Distribution
- Sharding strategies (hash, range, geographic)
- Partition keys and distribution
- Replica sets and consistency
- Read/write routing

### 2. ACID Transactions

#### Atomicity
```aql
BEGIN TRANSACTION;
  UPDATE accounts SET balance = balance - 100 WHERE id = 1;
  UPDATE accounts SET balance = balance + 100 WHERE id = 2;
COMMIT;
```

#### Consistency
- Constraints enforcement
- Referential integrity
- Data validation rules

#### Isolation Levels
- **Read Uncommitted**: Lowest isolation
- **Read Committed**: Default level
- **Repeatable Read**: Prevents non-repeatable reads
- **Serializable**: Highest isolation

#### Durability
- Write-ahead logging (WAL)
- Checkpoint mechanisms
- Recovery procedures

### 3. Basic AQL Queries

#### CRUD Operations

**Create (Insert)**
```aql
-- Insert single document
INSERT INTO users {
  name: "Alice Johnson",
  email: "alice@example.com",
  age: 28,
  created_at: NOW()
};

-- Insert multiple documents
INSERT INTO products VALUES
  {id: 1, name: "Laptop", price: 999.99},
  {id: 2, name: "Mouse", price: 29.99},
  {id: 3, name: "Keyboard", price: 79.99};
```

**Read (Select)**
```aql
-- Simple select
SELECT * FROM users WHERE age > 25;

-- With specific fields
SELECT name, email FROM users 
WHERE age BETWEEN 25 AND 35
ORDER BY name ASC;

-- With aggregation
SELECT department, COUNT(*) as employee_count,
       AVG(salary) as avg_salary
FROM employees
GROUP BY department
HAVING avg_salary > 50000;
```

**Update**
```aql
-- Update single document
UPDATE users 
SET age = 29, updated_at = NOW()
WHERE id = 123;

-- Conditional update
UPDATE products 
SET price = price * 0.9 
WHERE category = "electronics" AND stock > 100;
```

**Delete**
```aql
-- Delete with condition
DELETE FROM sessions 
WHERE last_activity < DATE_SUB(NOW(), INTERVAL 30 DAY);

-- Delete all (use with caution)
DELETE FROM temp_data;
```

#### Joins and Relationships

```aql
-- Inner join
SELECT u.name, o.order_id, o.total
FROM users u
INNER JOIN orders o ON u.id = o.user_id
WHERE o.status = "completed";

-- Left join
SELECT c.name, COUNT(o.id) as order_count
FROM customers c
LEFT JOIN orders o ON c.id = o.customer_id
GROUP BY c.id, c.name;

-- Graph traversal join
SELECT p.name, f.name as friend_name
FROM persons p
GRAPH_JOIN friends_edge ON p.id = friends_edge.from_id
GRAPH_JOIN persons f ON friends_edge.to_id = f.id
WHERE p.id = 1;
```

#### Functions and Operators

```aql
-- String functions
SELECT UPPER(name), LENGTH(email), SUBSTRING(phone, 1, 3)
FROM users;

-- Date functions
SELECT DATE_FORMAT(created_at, '%Y-%m-%d') as date,
       DATEDIFF(NOW(), created_at) as days_old
FROM orders;

-- Conditional expressions
SELECT name,
       CASE 
         WHEN age < 18 THEN 'Minor'
         WHEN age < 65 THEN 'Adult'
         ELSE 'Senior'
       END as age_group
FROM users;
```

### 4. Installation and Configuration

#### Installation Methods

**Docker Installation**
```bash
# Pull ThemisDB image
docker pull themisdb/themisdb:latest

# Run container
docker run -d \
  --name themisdb \
  -p 8529:8529 \
  -v themisdb-data:/var/lib/themisdb \
  themisdb/themisdb:latest
```

**Binary Installation**
```bash
# Download and extract
wget https://download.themisdb.com/latest/themisdb-linux.tar.gz
tar -xzf themisdb-linux.tar.gz
cd themisdb

# Start server
./bin/themisdb-server --config config/themisdb.conf
```

**Package Manager**
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install themisdb

# CentOS/RHEL
sudo yum install themisdb

# macOS
brew install themisdb
```

#### Configuration File

```ini
# themisdb.conf

[server]
endpoint = tcp://0.0.0.0:8529
authentication = true

[database]
directory = /var/lib/themisdb
wal-directory = /var/lib/themisdb/wal

[logging]
level = info
file = /var/log/themisdb/themisdb.log

[cache]
size = 2GB

[replication]
enabled = false
```

#### Directory Structure
```
/var/lib/themisdb/
├── data/           # Database files
├── wal/            # Write-ahead logs
├── temp/           # Temporary files
└── backups/        # Backup files

/var/log/themisdb/
└── themisdb.log    # Server logs

/etc/themisdb/
└── themisdb.conf   # Configuration
```

### 5. Basic Security

#### User Management

```aql
-- Create user
CREATE USER 'alice' IDENTIFIED BY 'SecureP@ssw0rd';

-- Grant privileges
GRANT READ, WRITE ON DATABASE mydb TO alice;

-- Grant specific table access
GRANT SELECT, INSERT, UPDATE ON mydb.orders TO alice;

-- Revoke privileges
REVOKE WRITE ON DATABASE mydb FROM alice;

-- Drop user
DROP USER alice;
```

#### Authentication Methods

**Password Authentication**
```bash
# Connect with username/password
themisdb-client --server localhost:8529 \
  --username alice \
  --password SecureP@ssw0rd
```

**JWT Token Authentication**
```bash
# Connect with JWT token
themisdb-client --server localhost:8529 \
  --auth-type jwt \
  --token eyJhbGciOiJIUzI1NiIs...
```

#### TLS/SSL Configuration

```ini
[ssl]
enabled = true
certificate = /etc/themisdb/ssl/server.crt
private-key = /etc/themisdb/ssl/server.key
ca-certificate = /etc/themisdb/ssl/ca.crt
```

#### Network Security

```ini
[server]
# Bind to specific interface
endpoint = tcp://192.168.1.10:8529

# Firewall rules (iptables)
# Allow only from specific IPs
```

---

## Study Materials and Resources

### Official Documentation
1. **ThemisDB User Manual**: Complete reference guide
2. **AQL Language Reference**: Query syntax and functions
3. **Installation Guide**: Platform-specific setup instructions
4. **Security Guide**: Authentication and authorization
5. **API Documentation**: REST and native APIs

### Online Courses
- **ThemisDB Fundamentals** (Self-paced, 20 hours)
- **Introduction to AQL** (Video series, 10 hours)
- **ThemisDB for Developers** (Interactive tutorials, 15 hours)

### Hands-On Labs
1. **Lab 1**: Installation and initial setup (2 hours)
2. **Lab 2**: Basic CRUD operations (3 hours)
3. **Lab 3**: Joins and aggregations (3 hours)
4. **Lab 4**: User management and security (2 hours)
5. **Lab 5**: Backup and restore (2 hours)

### Practice Environments
- **ThemisDB Cloud Sandbox**: Free 30-day access
- **Docker Development Environment**: Local setup
- **Interactive AQL Console**: Online query practice

### Community Resources
- **ThemisDB Forum**: Q&A and discussions
- **Stack Overflow**: Tag: themisdb
- **GitHub Issues**: Bug reports and feature requests
- **YouTube Channel**: Tutorial videos

### Recommended Books
- "Getting Started with ThemisDB" (Official Guide)
- "Multi-Model Databases Explained"
- "Modern Database Systems"

---

## Practice Exercises

### Exercise 1: Database Setup
**Objective**: Install and configure ThemisDB

**Tasks**:
1. Install ThemisDB using your preferred method
2. Start the database server
3. Connect using the CLI client
4. Create a new database named "practice"
5. Verify the installation

**Expected Outcome**: Successful connection to database

---

### Exercise 2: Basic Data Operations
**Objective**: Perform CRUD operations

**Tasks**:
1. Create a collection named "employees"
2. Insert 10 employee records with fields: id, name, department, salary
3. Query all employees in "Engineering" department
4. Update salary for employees with salary < 50000 (increase by 10%)
5. Delete employees with id > 100

**Expected Outcome**: Successfully manipulate employee data

---

### Exercise 3: Advanced Queries
**Objective**: Write complex queries

**Tasks**:
1. Create "departments" and "projects" collections
2. Establish relationships between employees, departments, and projects
3. Write query to find employees working on multiple projects
4. Calculate average salary by department
5. Find departments with no employees

**Expected Outcome**: Accurate query results

---

### Exercise 4: User Management
**Objective**: Configure security

**Tasks**:
1. Create three users: admin, developer, analyst
2. Grant appropriate permissions to each user
3. Test permissions by connecting as each user
4. Revoke write access from analyst
5. Document security configuration

**Expected Outcome**: Proper access control implemented

---

### Exercise 5: Backup and Recovery
**Objective**: Protect data

**Tasks**:
1. Create backup of practice database
2. Delete some data from the database
3. Restore from backup
4. Verify data integrity after restore
5. Automate backup script

**Expected Outcome**: Successful backup and restoration

---

## Sample Exam Questions

### Section 1: Architecture and Concepts

**Question 1**: What does ACID stand for in database transactions?
- A) Atomicity, Consistency, Isolation, Durability
- B) Authentication, Confidentiality, Integrity, Durability
- C) Atomicity, Confidentiality, Isolation, Distribution
- D) Authentication, Consistency, Integrity, Distribution

**Answer**: A

---

**Question 2**: Which isolation level prevents dirty reads but allows non-repeatable reads?
- A) Read Uncommitted
- B) Read Committed
- C) Repeatable Read
- D) Serializable

**Answer**: B

---

**Question 3**: In ThemisDB's multi-model architecture, which model is best suited for storing social network connections?
- A) Document Model
- B) Key-Value Model
- C) Graph Model
- D) Time-Series Model

**Answer**: C

---

**Question 4**: What is the purpose of sharding in ThemisDB?
- A) To encrypt data at rest
- B) To distribute data across multiple nodes for scalability
- C) To compress data for storage efficiency
- D) To validate data integrity

**Answer**: B

---

**Question 5**: Which component is responsible for parsing and optimizing AQL queries?
- A) Storage Engine
- B) Query Engine
- C) Transaction Manager
- D) Replication Layer

**Answer**: B

---

### Section 2: AQL Query Language

**Question 6**: What is the correct syntax to insert a document into a collection?
- A) `ADD INTO users {name: "John"};`
- B) `INSERT users VALUES {name: "John"};`
- C) `INSERT INTO users {name: "John"};`
- D) `CREATE users {name: "John"};`

**Answer**: C

---

**Question 7**: Which clause is used to filter aggregated results?
- A) WHERE
- B) FILTER
- C) HAVING
- D) CONDITION

**Answer**: C

---

**Question 8**: What does the following query return?
```aql
SELECT department, COUNT(*) as total
FROM employees
GROUP BY department
ORDER BY total DESC
LIMIT 3;
```
- A) Top 3 employees by department
- B) Top 3 departments by employee count
- C) First 3 departments alphabetically
- D) Random 3 departments

**Answer**: B

---

**Question 9**: How do you perform a LEFT JOIN in AQL?
- A) `SELECT * FROM a, b WHERE a.id = b.id;`
- B) `SELECT * FROM a LEFT JOIN b ON a.id = b.id;`
- C) `SELECT * FROM a OUTER JOIN b ON a.id = b.id;`
- D) `SELECT * FROM a JOIN b LEFT ON a.id = b.id;`

**Answer**: B

---

**Question 10**: Which function returns the current timestamp?
- A) CURRENT_TIME()
- B) NOW()
- C) TIMESTAMP()
- D) GETDATE()

**Answer**: B

---

### Section 3: Installation and Configuration

**Question 11**: Which configuration parameter controls the memory allocated to the cache?
- A) `[cache] memory`
- B) `[cache] size`
- C) `[server] cache-size`
- D) `[database] cache-memory`

**Answer**: B

---

**Question 12**: What is the default port for ThemisDB server?
- A) 3306
- B) 5432
- C) 8529
- D) 27017

**Answer**: C

---

**Question 13**: Which directory stores the write-ahead logs by default?
- A) `/var/lib/themisdb/data`
- B) `/var/lib/themisdb/wal`
- C) `/var/log/themisdb`
- D) `/etc/themisdb/logs`

**Answer**: B

---

**Question 14**: How do you start ThemisDB in Docker?
- A) `docker start themisdb`
- B) `docker run themisdb/themisdb`
- C) `docker exec themisdb start`
- D) `docker compose up themisdb`

**Answer**: B

---

**Question 15**: Which command checks the ThemisDB server status?
- A) `themisdb status`
- B) `themisdb-server --status`
- C) `systemctl status themisdb`
- D) All of the above

**Answer**: C (on Linux with systemd)

---

### Section 4: Security

**Question 16**: What is the correct syntax to create a new user?
- A) `NEW USER 'alice' PASSWORD 'secret';`
- B) `CREATE USER 'alice' IDENTIFIED BY 'secret';`
- C) `ADD USER alice WITH PASSWORD 'secret';`
- D) `INSERT USER alice PASSWORD='secret';`

**Answer**: B

---

**Question 17**: Which privilege allows a user to read data from a database?
- A) SELECT
- B) READ
- C) VIEW
- D) GET

**Answer**: B (In ThemisDB's privilege model)

---

**Question 18**: What is the purpose of TLS/SSL in ThemisDB?
- A) To compress data
- B) To encrypt data in transit
- C) To authenticate users
- D) To backup data

**Answer**: B

---

**Question 19**: Which authentication method uses tokens?
- A) Basic Authentication
- B) Certificate Authentication
- C) JWT Authentication
- D) Kerberos

**Answer**: C

---

**Question 20**: What should you do after creating a new user with administrative privileges?
- A) Nothing, user is ready to use
- B) Restart the database
- C) Change the default password
- D) Grant specific permissions

**Answer**: C (Security best practice)

---

### Section 5: Operations

**Question 21**: Which command creates a backup of a database?
- A) `BACKUP DATABASE mydb TO '/backup/mydb.bak';`
- B) `DUMP DATABASE mydb > /backup/mydb.dump;`
- C) `EXPORT DATABASE mydb TO '/backup/mydb';`
- D) `COPY DATABASE mydb TO '/backup/mydb';`

**Answer**: A

---

**Question 22**: What is the purpose of the write-ahead log (WAL)?
- A) To improve query performance
- B) To ensure durability of transactions
- C) To compress database files
- D) To replicate data to other nodes

**Answer**: B

---

**Question 23**: Which metric indicates potential performance issues?
- A) Low CPU usage
- B) High query latency
- C) Low disk I/O
- D) High available memory

**Answer**: B

---

**Question 24**: How often should you perform database backups in production?
- A) Never, databases auto-backup
- B) Only before major updates
- C) Regularly based on RPO requirements
- D) Only when requested

**Answer**: C

---

**Question 25**: What is the first step in troubleshooting a connection issue?
- A) Restart the database
- B) Check server logs
- C) Reinstall ThemisDB
- D) Contact support

**Answer**: B

---

### Scenario-Based Questions

**Question 26**: You need to design a schema for an e-commerce application storing products, customers, and orders. Which data model(s) should you use?
- A) Document model only
- B) Graph model only
- C) Document model for products/customers, graph model for recommendations
- D) Key-value model for everything

**Answer**: C

---

**Question 27**: Your application requires reading the same data multiple times within a transaction with guaranteed consistency. Which isolation level should you use?
- A) Read Uncommitted
- B) Read Committed
- C) Repeatable Read
- D) Any level works

**Answer**: C

---

**Question 28**: You notice slow query performance on a frequently-accessed table. What should you check first?
- A) Server hardware
- B) Network bandwidth
- C) Index configuration
- D) Replication lag

**Answer**: C

---

**Question 29**: A user reports they cannot connect to ThemisDB. They get "Authentication failed" error. What's the most likely cause?
- A) Server is down
- B) Incorrect username or password
- C) Network firewall blocking connection
- D) Database is corrupted

**Answer**: B

---

**Question 30**: You need to grant read-only access to an external reporting tool. What's the best approach?
- A) Share admin credentials
- B) Create a user with READ privileges only
- C) Disable authentication temporarily
- D) Export data to CSV files

**Answer**: B

---

## Certification Criteria

### Exam Requirements

**To pass the ThemisDB Fundamentals Certification, you must:**

1. **Score 70% or higher** (18/25 questions minimum)
2. **Complete exam within 90 minutes**
3. **Answer all questions** (no partial credit)
4. **Adhere to exam policies** (no cheating, no external resources)

### Evaluation Breakdown

| Topic Area | Questions | Weight |
|-----------|-----------|--------|
| Architecture and Concepts | 5-6 | 20% |
| Data Models | 3-4 | 15% |
| AQL Query Language | 6-8 | 25% |
| Installation and Configuration | 4-5 | 15% |
| Security Fundamentals | 4-5 | 15% |
| Operations Basics | 2-3 | 10% |
| **Total** | **25-30** | **100%** |

### Scoring System

- **Multiple Choice**: 1 point per question
- **Multiple Select**: All correct answers required for credit
- **Scenario-Based**: 1-2 points based on complexity

### Pass/Fail Notification

- Results available immediately upon exam completion
- Detailed score report provided
- Performance breakdown by topic area
- Recommended study areas for failed attempts

---

## Exam Format and Duration

### Exam Structure

**Total Duration**: 90 minutes

**Breakdown**:
- Tutorial and agreement: 5 minutes
- Exam questions: 75 minutes
- Survey: 5 minutes
- Buffer: 5 minutes

### Question Distribution

```
25-30 Total Questions
├── 15-18 Multiple Choice (single answer)
├── 5-7 Multiple Select (multiple correct answers)
└── 5-6 Scenario-Based (applied knowledge)
```

### Exam Environment

**Online Proctored**:
- Webcam and microphone required
- Screen recording active
- Live proctor monitoring
- Secure browser required
- ID verification at start

**Testing Center**:
- Government-issued ID required
- No personal items allowed
- Provided: scratch paper, pencil
- Quiet, proctored environment

### During the Exam

**Allowed**:
- Note-taking on provided materials
- Calculator (provided in software)
- Bathroom breaks (time continues)

**Not Allowed**:
- External resources (documentation, books)
- Communication with others
- Mobile phones or devices
- Notes or cheat sheets
- Screen capture or recording

### After the Exam

**Immediate**:
- Pass/Fail notification
- Preliminary score

**Within 48 hours**:
- Digital certificate
- Verification badge
- Detailed score report
- Certificate ID for verification

---

## Certification Benefits

### Professional Recognition

1. **Digital Badge**: Shareable credential with unique verification ID
2. **Certificate**: PDF and printed versions available
3. **Transcript**: Official record of achievement
4. **Directory Listing**: Optional profile in certified professionals database

### Career Advantages

- **Resume Enhancement**: Industry-recognized credential
- **Job Opportunities**: Access to ThemisDB job board
- **Salary Premium**: Average 15% increase for certified professionals
- **Career Growth**: Foundation for advanced certifications

### Learning Resources

- **Certified Community**: Private forum access
- **Updates**: Early notification of new features
- **Discounts**: 20% off training courses
- **Events**: Invitations to certification holder events

### Employer Benefits

- **Verification**: Instant credential verification for employers
- **Skills Validation**: Proven ThemisDB competency
- **Professional Development**: Continuing education requirements
- **Quality Assurance**: Standardized knowledge baseline

---

## Renewal Requirements

### Certification Validity
**2 years** from certification date

### Renewal Options

#### Option 1: Recertification Exam
- **Duration**: 45 minutes (50% of original)
- **Questions**: 12-15 questions
- **Cost**: $75 (50% discount)
- **Focus**: New features and updates since last certification

#### Option 2: Continuing Education
Earn **20 CEUs** over 2 years:

| Activity | CEUs |
|----------|------|
| Advanced certification (TQE, TOC, TSC) | 20 (auto-renews TDF) |
| Official training course completion | 5-10 |
| Conference attendance | 2-5 |
| Webinar participation | 1-2 |
| Technical blog post | 2 |
| Open-source contribution | 3-5 |
| Speaking engagement | 5-10 |

#### Option 3: Higher Certification
- Earning TQE, TOC, or TSC automatically renews TDF
- No additional renewal exam required

### Renewal Process

1. **Log into certification portal**
2. **Choose renewal method**
3. **Complete requirements**
4. **Submit documentation** (if CEU path)
5. **Receive updated credential** (within 5 business days)

### Lapsed Certification

If certification expires:
- **Grace Period**: 3 months with 10% late fee
- **After Grace**: Must retake full exam
- **No Transfer**: CEUs don't transfer to new exam

---

## Next Steps After Certification

### Immediate Actions

1. **Download Certificate**: Save PDF and print copies
2. **Add Badge to LinkedIn**: Share your achievement
3. **Update Resume**: Include certification details
4. **Join Community**: Access certified professionals forum
5. **Plan Next Certification**: Consider TQE, TOC, or TSC

### Career Path Options

**For Developers**:
- Pursue **Query Expert Certification (TQE)**
- Focus on application development
- Master advanced AQL techniques

**For DBAs**:
- Pursue **Operations Certification (TOC)**
- Focus on production management
- Master monitoring and optimization

**For Security Roles**:
- Pursue **Security Certification (TSC)**
- Focus on compliance and hardening
- Master security best practices

**For Architects**:
- Pursue **all advanced certifications**
- Design enterprise solutions
- Lead ThemisDB initiatives

### Continuing Education

- **Subscribe**: ThemisDB newsletter and blog
- **Participate**: Community forums and discussions
- **Practice**: Regular hands-on work with ThemisDB
- **Share**: Write articles, present at meetups
- **Contribute**: Open-source contributions

---

## Preparation Timeline

### 6-Week Study Plan

**Week 1-2: Architecture and Concepts**
- Read ThemisDB architecture documentation
- Watch video tutorials on multi-model databases
- Complete Lab 1: Installation
- Study time: 10-12 hours

**Week 3: AQL Fundamentals**
- Learn AQL syntax and basics
- Practice CRUD operations
- Complete Lab 2: Basic Operations
- Study time: 8-10 hours

**Week 4: Advanced AQL**
- Master joins and aggregations
- Practice complex queries
- Complete Lab 3: Advanced Queries
- Study time: 8-10 hours

**Week 5: Security and Operations**
- Learn user management
- Practice backup/restore
- Complete Labs 4 & 5
- Study time: 8-10 hours

**Week 6: Review and Practice**
- Take practice exams
- Review weak areas
- Final hands-on exercises
- Study time: 10-12 hours

**Total Study Time**: 44-54 hours over 6 weeks

---

## Support and Resources

### Certification Support

**Email**: certification@themisdb.com  
**Phone**: +1-555-THEMISDB  
**Hours**: Monday-Friday, 9 AM - 5 PM EST

### Technical Support

**Forum**: community.themisdb.com  
**Slack**: themisdb-cert.slack.com  
**Stack Overflow**: Tag: themisdb

### Exam Support

**Before Exam**: cert-support@themisdb.com  
**During Exam**: Use chat feature in exam portal  
**After Exam**: results@themisdb.com

### Accessibility

We provide accommodations for candidates with disabilities:
- Extended time
- Screen readers
- Alternative formats
- Contact: accessibility@themisdb.com

---

## Frequently Asked Questions

**Q: How difficult is the TDF exam?**  
A: With proper preparation (40-60 hours of study), most candidates with basic database experience pass on their first attempt. The 70% passing score is achievable.

**Q: Can I use documentation during the exam?**  
A: No, the exam is closed-book. You must rely on your knowledge and memory.

**Q: How long until I receive my certificate?**  
A: Digital certificate and badge are issued within 48 hours of passing.

**Q: Can I retake if I fail?**  
A: Yes, after a 14-day waiting period. Retake fee is $75.

**Q: Is hands-on experience required?**  
A: While not mandatory, 3-6 months of hands-on experience significantly improves your chances of passing.

**Q: Are there prerequisites?**  
A: No formal prerequisites, but basic database knowledge is recommended.

**Q: How is this different from other database certifications?**  
A: TDF focuses specifically on ThemisDB's unique multi-model capabilities and AQL query language.

**Q: Can I take the exam in my native language?**  
A: Currently English only. Spanish, Chinese, and Japanese coming in 2025.

**Q: What if I have technical issues during the exam?**  
A: Use the chat feature immediately. Your exam will be paused while issues are resolved.

**Q: Do I need to renew my certification?**  
A: Yes, every 2 years to stay current with ThemisDB developments.

---

## Success Tips

### Before the Exam

1. **Study Consistently**: 1-2 hours daily is better than cramming
2. **Practice Hands-On**: Actually install and use ThemisDB
3. **Take Practice Exams**: Identify weak areas early
4. **Join Study Groups**: Learn from others
5. **Get Enough Sleep**: Be well-rested on exam day

### During the Exam

1. **Read Carefully**: Understand what each question asks
2. **Manage Time**: Don't spend too long on any question
3. **Flag for Review**: Mark uncertain questions to revisit
4. **Eliminate Wrong Answers**: Narrow down options
5. **Stay Calm**: Take deep breaths if feeling anxious

### After the Exam

1. **Review Score Report**: Understand performance by topic
2. **If Failed**: Study weak areas and retake after 14 days
3. **If Passed**: Share achievement and plan next steps
4. **Provide Feedback**: Help improve the certification program
5. **Stay Current**: Continue learning about ThemisDB

---

**Ready to get certified? Register today and start your journey!**

[Register for TDF Certification →](https://certify.themisdb.com/register/tdf)

---

*Last Updated: April 2026*  
*Version: 1.0*  
*© 2025 ThemisDB. All rights reserved.*
